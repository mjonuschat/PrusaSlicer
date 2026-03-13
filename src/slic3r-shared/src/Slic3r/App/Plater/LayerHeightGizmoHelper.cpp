#include "Slic3r/App/Plater/LayerHeightGizmoHelper.hpp"

#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/TextureManager.hpp"
#include "Slic3r/App/Scene/AabbRaycastNodeComponent.hpp"
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Yoga/LayerHeightProfileControl.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/LayerHeight.hpp"
#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Domain/Color.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Constants.hpp"
#include "Slic3r/Domain/LayerHeightProfile.hpp"
#include "Slic3r/Domain/ModelObject.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/Transformation.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/BedInstance.hpp"

#include "libslic3r/ExtruderCandidates.hpp"
#include "libslic3r/Slicing.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Slic3r;
using namespace Slic3r::App::Yoga;
using namespace Slic3r::Biz;

using Slic3r::Biz::Algorithms::LayerHeight::ProfileFromRangesParams;
using Slic3r::Biz::Scene::ObjectSelection;
using Slic3r::Domain::BoundingBox3d;
using Slic3r::Domain::ColorRGBA;
using Slic3r::Domain::ConfigContainer;
using Slic3r::Domain::ConfigItem;
using Slic3r::Domain::ConfigPack;
using Slic3r::Domain::ConfigPackFDM;
using Slic3r::Domain::LayerConfigRanges;
using Slic3r::Domain::LayerHeightRange;
using Slic3r::Domain::LayerZRange;
using Slic3r::Domain::LayerZRanges;
using Slic3r::Domain::ModelObject;
using Slic3r::Domain::Project;
using Slic3r::Domain::Vec3crd;
using Slic3r::Domain::Vec3d;
using Slic3r::Domain::Vec3f;
using Slic3r::Domain::VolumeSettings;
using Slic3r::Domain::ZHeightPairs;

namespace Slic3r::App::Plater {

const constexpr double LAYERS_HEIGHT_PROFILE_VALID_THRESHOLD = 0.001;
const constexpr size_t LAYERS_TEXTURE_WIDTH                  = 1'024;
const constexpr size_t LAYERS_TEXTURE_HEIGHT                 = 1'024;
const constexpr size_t LAYERS_TEXTURE_LEVELS                 = 2;

const constexpr double HEIGHT_RANGE_HEIGHT_EPSILON       = 0.0001;
const constexpr double HEIGHT_RANGE_DEFAULT_HEIGHT       = 2.;
const constexpr double HEIGHT_RANGE_PLANE_MIN_SEPARATION = Domain::EPSILON;
const ColorRGBA HEIGHT_RANGE_PLANE_COLOR{0.75f, 0.75f, 0.75f, 0.5f};
const ColorRGBA HEIGHT_RANGE_PLANE_HOVER_COLOR{0.95f, 0.95f, 0.95f, 0.5f};

LayerHeightParams compute_layer_height_params(
    const ObjectSelection& object_selection,
    const Project& project,
    const ConfigContainer& config_container,
    const Domain::BedRef& bed_ref
)
{
    const ModelObject& model_object =
        *project.find_object_by_id(object_selection.elements.front().object_id);
    const ConfigPack config_pack    = config_container.print_config();
    const ConfigPackFDM& fdm_config = std::get<ConfigPackFDM>(config_pack);

    ASSERT(bed_ref.config_container_id == config_container.id().id);
    const Domain::BedInstance& bed_instance{
        config_container.find_bed_instance(bed_ref.instance_id)
    };

    const std::vector<unsigned> extruder_candidates =
        Slicing::get_extruder_candidates(project.model(), fdm_config, bed_instance);

    const SlicingParameters slicing_parameters = SlicingParameters::create_from_config(
        fdm_config,
        model_object.object_settings,
        model_object.max_z(),
        extruder_candidates,
        config_container.selected_preset().hw_config
    );

    LayerHeightParams params = {
        .layer_height                    = slicing_parameters.layer_height,
        .min_layer_height                = slicing_parameters.min_layer_height,
        .max_layer_height                = slicing_parameters.max_layer_height,
        .first_object_layer_height       = slicing_parameters.first_object_layer_height,
        .first_object_layer_height_fixed = slicing_parameters.first_object_layer_height_fixed(),
        .object_print_z_height           = slicing_parameters.object_print_z_height(),
        .object_print_z_uncompensated_height =
            slicing_parameters.object_print_z_uncompensated_height(),
        .object_shrinkage_compensation_z = slicing_parameters.object_shrinkage_compensation_z
    };

    params.layer_height_profile = compute_layer_height_profile(
        params,
        model_object.layer_config_ranges,
        model_object.layer_height_profile.get()
    );

    return params;
}

bool is_valid_layer_height_profile(
    const ZHeightPairs& layer_height_profile,
    const double object_print_z_uncompensated_height
)
{
    return !layer_height_profile.empty()
        && std::abs(layer_height_profile.back().z - object_print_z_uncompensated_height)
        <= LAYERS_HEIGHT_PROFILE_VALID_THRESHOLD;
}

ZHeightPairs compute_layer_height_profile(
    const LayerHeightParams& params,
    const LayerConfigRanges& layer_config_ranges,
    const ZHeightPairs& layer_height_profile
)
{
    if (is_valid_layer_height_profile(
            layer_height_profile,
            params.object_print_z_uncompensated_height
        ))
    {
        return layer_height_profile;
    }

    const ProfileFromRangesParams profile_from_ranges_params{
        .layer_height                        = params.layer_height,
        .first_object_layer_height           = params.first_object_layer_height,
        .object_print_z_height               = params.object_print_z_height,
        .object_print_z_uncompensated_height = params.object_print_z_uncompensated_height,
        .first_object_layer_height_fixed     = params.first_object_layer_height_fixed
    };
    return Algorithms::LayerHeight::layer_height_profile_from_ranges(
        profile_from_ranges_params,
        layer_config_ranges
    );
}

HeightRangeEntries create_height_ranges_from_config(
    const LayerConfigRanges& layer_config_ranges,
    const double default_layer_height
)
{
    HeightRangeEntries height_ranges;
    for (const std::pair<const LayerHeightRange, VolumeSettings>& entry : layer_config_ranges) {
        const LayerHeightRange& range         = entry.first;
        const VolumeSettings& volume_settings = entry.second;
        const std::optional<ConfigItem> layer_height_override =
            volume_settings.overrides.get("layer_height");
        const double layer_height = layer_height_override.has_value() ?
            layer_height_override->get<double>() :
            default_layer_height;

        height_ranges.push_back(
            HeightRangeEntry{
                .min_z        = range.first,
                .max_z        = range.second,
                .layer_height = layer_height
            }
        );
    }

    return height_ranges;
}

/**
 * Parameters for generating layer height texture.
 */
struct LayerHeightTextureParams
{
    double min_layer_height{0.};
    double max_layer_height{0.};
    /**
     * Default layer height.
     */
    double layer_height{0.};
    /**
     * Total Z height (with applied shrinkage compensation).
     */
    double object_height{0.};
};

/**
 * Produce a 1D texture packed into a 2D texture describing in the RGBA format the planned object layers.
 *
 * @return Returns the number of cells used by the texture of the 0th LOD level.
 */
int generate_layer_height_texture(
    const LayerHeightTextureParams& params,
    const LayerZRanges& layers,
    void* data,
    int rows,
    int cols,
    bool level_of_detail_2nd_level
)
{
    // https://github.com/aschn/gnuplot-colorbrewer
    std::vector<Vec3crd> palette_raw;
    palette_raw.emplace_back(0x01A, 0x098, 0x050);
    palette_raw.emplace_back(0x066, 0x0BD, 0x063);
    palette_raw.emplace_back(0x0A6, 0x0D9, 0x06A);
    palette_raw.emplace_back(0x0D9, 0x0F1, 0x0EB);
    palette_raw.emplace_back(0x0FE, 0x0E6, 0x0EB);
    palette_raw.emplace_back(0x0FD, 0x0AE, 0x061);
    palette_raw.emplace_back(0x0F4, 0x06D, 0x043);
    palette_raw.emplace_back(0x0D7, 0x030, 0x027);

    // 2nd LOD level data start.
    unsigned char* data1 = reinterpret_cast<unsigned char*>(data) + rows * cols * 4;
    int ncells           = std::min(
        (cols - 1) * rows,
        int(ceil(16. * (params.object_height / params.min_layer_height)))
    );
    int ncells1       = ncells / 2;
    int cols1         = cols / 2;
    double z_to_cell  = double(ncells - 1) / params.object_height;
    double cell_to_z  = params.object_height / double(ncells - 1);
    double z_to_cell1 = double(ncells1 - 1) / params.object_height;
    // For color scaling.
    double hscale = 2.f
        * std::max(params.max_layer_height - params.layer_height,
                   params.layer_height - params.min_layer_height);
    if (hscale == 0) {
        // All layers have the same height. Provide some height scale to avoid division by zero.
        hscale = params.layer_height;
    }

    for (const LayerZRange& layer_z_range : layers) {
        const double lo  = layer_z_range.bottom_z;
        const double mid = layer_z_range.middle_z();
        assert(mid <= params.object_height);
        const double h  = layer_z_range.height();
        const double hi = std::min(layer_z_range.top_z, params.object_height);
        int cell_first  = std::clamp(int(ceil(lo * z_to_cell)), 0, ncells - 1);
        int cell_last   = std::clamp(int(floor(hi * z_to_cell)), 0, ncells - 1);
        for (int cell = cell_first; cell <= cell_last; ++cell) {
            double idxf = (0.5 * hscale + (h - params.layer_height))
                * double(palette_raw.size() - 1)
                / hscale;
            int idx1              = std::clamp(int(floor(idxf)), 0, int(palette_raw.size() - 1));
            int idx2              = std::min(int(palette_raw.size() - 1), idx1 + 1);
            double t              = idxf - double(idx1);
            const Vec3crd& color1 = palette_raw[idx1];
            const Vec3crd& color2 = palette_raw[idx2];
            double z              = cell_to_z * double(cell);
            assert(lo - Domain::EPSILON <= z && z <= hi + Domain::EPSILON);
            // Intensity profile to visualize the layers.
            double intensity = cos(std::numbers::pi * 0.7 * (mid - z) / h);
            // Color mapping from layer height to RGB.
            Vec3d color(
                intensity * std::lerp(double(color1.x()), double(color2.x()), t),
                intensity * std::lerp(double(color1.y()), double(color2.y()), t),
                intensity * std::lerp(double(color1.z()), double(color2.z()), t)
            );
            int row = cell / (cols - 1);
            int col = cell - row * (cols - 1);
            assert(row >= 0 && row < rows);
            assert(col >= 0 && col < cols);
            unsigned char* ptr = (unsigned char*) data + (row * cols + col) * 4;
            ptr[0]             = (unsigned char) std::clamp(int(floor(color.x() + 0.5)), 0, 255);
            ptr[1]             = (unsigned char) std::clamp(int(floor(color.y() + 0.5)), 0, 255);
            ptr[2]             = (unsigned char) std::clamp(int(floor(color.z() + 0.5)), 0, 255);
            ptr[3]             = 255;
            if (col == 0 && row > 0) {
                // Duplicate the first value in a row as a last value of the preceding row.
                ptr[-4] = ptr[0];
                ptr[-3] = ptr[1];
                ptr[-2] = ptr[2];
                ptr[-1] = ptr[3];
            }
        }

        if (level_of_detail_2nd_level) {
            cell_first = std::clamp(int(ceil(lo * z_to_cell1)), 0, ncells1 - 1);
            cell_last  = std::clamp(int(floor(hi * z_to_cell1)), 0, ncells1 - 1);
            for (int cell = cell_first; cell <= cell_last; ++cell) {
                double idxf = (0.5 * hscale + (h - params.layer_height))
                    * double(palette_raw.size() - 1)
                    / hscale;
                int idx1 = std::clamp(int(floor(idxf)), 0, int(palette_raw.size() - 1));
                int idx2 = std::min(int(palette_raw.size() - 1), idx1 + 1);
                double t = idxf - double(idx1);
                const Vec3crd& color1 = palette_raw[idx1];
                const Vec3crd& color2 = palette_raw[idx2];
                // Color mapping from layer height to RGB.
                Vec3d color(
                    std::lerp(double(color1.x()), double(color2.x()), t),
                    std::lerp(double(color1.y()), double(color2.y()), t),
                    std::lerp(double(color1.z()), double(color2.z()), t)
                );
                int row = cell / (cols1 - 1);
                int col = cell - row * (cols1 - 1);
                assert(row >= 0 && row < rows / 2);
                assert(col >= 0 && col < cols / 2);
                unsigned char* ptr = data1 + (row * cols1 + col) * 4;
                ptr[0] = (unsigned char) std::clamp(int(floor(color.x() + 0.5)), 0, 255);
                ptr[1] = (unsigned char) std::clamp(int(floor(color.y() + 0.5)), 0, 255);
                ptr[2] = (unsigned char) std::clamp(int(floor(color.z() + 0.5)), 0, 255);
                ptr[3] = 255;
                if (col == 0 && row > 0) {
                    // Duplicate the first value in a row as a last value of the preceding row.
                    ptr[-4] = ptr[0];
                    ptr[-3] = ptr[1];
                    ptr[-2] = ptr[2];
                    ptr[-1] = ptr[3];
                }
            }
        }
    }

    // Returns the number of cells of the 0th LOD level.
    return ncells;
}

LayerHeightTexture generate_layer_height_texture(
    const LayerZRanges& layers,
    const double min_layer_height,
    const double max_layer_height,
    const double layer_height,
    const double object_height
)
{
    LayerHeightTextureParams params = {
        .min_layer_height = min_layer_height,
        .max_layer_height = max_layer_height,
        .layer_height     = layer_height,
        .object_height    = object_height
    };

    LayerHeightTexture layer_height_texture = {
        .width  = LAYERS_TEXTURE_WIDTH,
        .height = LAYERS_TEXTURE_HEIGHT,
        .levels = LAYERS_TEXTURE_LEVELS
    };
    layer_height_texture.allocate();
    layer_height_texture.cells = generate_layer_height_texture(
        params,
        layers,
        layer_height_texture.data.data(),
        LAYERS_TEXTURE_HEIGHT,
        LAYERS_TEXTURE_WIDTH,
        LAYERS_TEXTURE_LEVELS == 2
    );

    return layer_height_texture;
}

std::optional<LayerHeightRange> compute_new_height_range(
    const std::optional<LayerHeightRange>& selected,
    const std::optional<LayerHeightRange>& next_after_selected,
    const double max_existing_z,
    const double object_max_z,
    const double min_layer_height
)
{
    if (selected.has_value()) {
        if (!next_after_selected.has_value()) {
            if (selected->second >= object_max_z) {
                return std::nullopt;
            }

            return LayerHeightRange{
                selected->second,
                std::min(selected->second + HEIGHT_RANGE_DEFAULT_HEIGHT, object_max_z)
            };
        }

        const LayerHeightRange& next_range = *next_after_selected;
        if (selected->second == next_range.first) {
            const double z_delta = next_range.second - next_range.first;

            if (z_delta < min_layer_height + min_layer_height - HEIGHT_RANGE_HEIGHT_EPSILON) {
                return std::nullopt;
            }

            const double middle_z = (min_layer_height > 0.5 * z_delta) ?
                next_range.second - min_layer_height :
                next_range.first + std::max(min_layer_height, 0.5 * z_delta);

            return LayerHeightRange{selected->second, middle_z};
        }

        if (next_range.first - selected->second >= min_layer_height - HEIGHT_RANGE_HEIGHT_EPSILON) {
            return LayerHeightRange{selected->second, next_range.first};
        }

        return std::nullopt;
    }

    if (max_existing_z > 0.) {
        if (max_existing_z >= object_max_z) {
            return std::nullopt;
        }

        return LayerHeightRange{
            max_existing_z,
            std::min(max_existing_z + HEIGHT_RANGE_DEFAULT_HEIGHT, object_max_z)
        };
    }

    return LayerHeightRange{0., std::min(HEIGHT_RANGE_DEFAULT_HEIGHT, object_max_z)};
}

void LayerHeightMaterialWrapper::init(Render::Device& device)
{
    m_texture = device.context().texture_manager().get_or_create_dynamic(
        "variable_layer_height",
        Domain::PixelFormat::RGBA8,
        LAYERS_TEXTURE_WIDTH,
        LAYERS_TEXTURE_HEIGHT
    );

    m_texture->set_filtering(
        Render::TextureMinFilter::MipMapLinearNearest,
        Render::TextureMagFilter::Linear
    );
    m_texture->set_wrap_s(Render::TextureWrap::ClampToEdge);
    m_texture->set_wrap_t(Render::TextureWrap::ClampToEdge);

    m_material = Render::Material{}
                     .set_shader(device.context().shader_manager().shader("variable_layer_height"))
                     .set_texture(0, m_texture)
                     .set_uniform("z_texture", 0)
                     .set_uniform("z_to_texture_row", 0.f)
                     .set_uniform(
                         "z_texture_row_to_normalized",
                         1.f / static_cast<float>(LAYERS_TEXTURE_HEIGHT)
                     )
                     .set_uniform("z_cursor", 0.f)
                     .set_uniform("z_cursor_band_width", 2.f)
                     .set_uniform("object_max_z", 0.f);

    // Non-zero object_max_z was used for layer height rendering in the side panel. Keeping it for now, may revisit later.
}

void LayerHeightMaterialWrapper::reset()
{
    m_material = Render::Material();
    m_texture.reset();
}

const Render::Material& LayerHeightMaterialWrapper::material() const
{
    return m_material;
}

void LayerHeightMaterialWrapper::set_layers(
    const LayerZRanges& layers,
    const double min_layer_height,
    const double max_layer_height,
    const double layer_height,
    const double object_height,
    const float object_max_z
)
{
    const LayerHeightTexture layer_height_texture = generate_layer_height_texture(
        layers,
        min_layer_height,
        max_layer_height,
        layer_height,
        object_height
    );

    // LOD 0 (full resolution).
    m_texture->set_data(
        Domain::PixelFormat::RGBA8,
        0,
        static_cast<int>(layer_height_texture.width),
        static_cast<int>(layer_height_texture.height),
        layer_height_texture.data.data(),
        layer_height_texture.width * layer_height_texture.height * 4
    );

    ASSERT(layer_height_texture.levels <= 2);

    // LOD 1 (half resolution).
    if (layer_height_texture.levels >= 2) {
        const int half_width  = static_cast<int>(layer_height_texture.width) / 2;
        const int half_height = static_cast<int>(layer_height_texture.height) / 2;
        m_texture->set_data(
            Domain::PixelFormat::RGBA8,
            1,
            half_width,
            half_height,
            layer_height_texture.data.data()
                + layer_height_texture.width * layer_height_texture.height * 4,
            half_width * half_height * 4
        );
    }

    const float z_texture_row_to_normalized = 1.f / float(layer_height_texture.height);
    const float z_to_texture_row            = static_cast<float>(layer_height_texture.cells - 1)
        / (static_cast<float>(layer_height_texture.width) * object_max_z);

    m_material.set_texture(0, m_texture)
        .set_uniform("z_texture_row_to_normalized", z_texture_row_to_normalized)
        .set_uniform("z_to_texture_row", z_to_texture_row);
}

void LayerHeightMaterialWrapper::set_cursor_z(const float cursor_z)
{
    m_material.set_uniform("z_cursor", cursor_z);
}

void LayerHeightMaterialWrapper::set_cursor_band_width(const float band_width)
{
    m_material.set_uniform("z_cursor_band_width", band_width);
}

void HeightRangePlanesWrapper::init(
    Render::Device& device,
    Scene::Scene& scene,
    const ModelObject& model_object
)
{
    ASSERT(m_main_node == nullptr);

    Scene::NodeBuilder main_node_builder{scene};
    main_node_builder.set_debug_name("HeightRangePlanes - Main node");
    main_node_builder.set_tag(HeightRangeNodeTag());

    main_node_builder.child(
        [&](Scene::NodeBuilder& node_builder)
        {
            this->build_plane_node(
                device,
                node_builder,
                HeightRangePlaneNodeTag::PlaneType::Min,
                model_object
            );
        }
    );
    main_node_builder.child(
        [&](Scene::NodeBuilder& node_builder)
        {
            this->build_plane_node(
                device,
                node_builder,
                HeightRangePlaneNodeTag::PlaneType::Max,
                model_object
            );
        }
    );

    std::unique_ptr<Scene::Node> main_node = main_node_builder.build();
    m_main_node                            = main_node.get();
    scene.add_child(main_node.release(), &scene.root());

    m_min_plane_node = m_main_node->query_first(
        [](const Scene::Node* n) -> bool
        {
            const HeightRangePlaneNodeTag* tag = n->tag_of_type<HeightRangePlaneNodeTag>();
            return tag != nullptr && tag->plane_type == HeightRangePlaneNodeTag::PlaneType::Min;
        },
        true
    );
    m_max_plane_node = m_main_node->query_first(
        [](const Scene::Node* n) -> bool
        {
            const HeightRangePlaneNodeTag* tag = n->tag_of_type<HeightRangePlaneNodeTag>();
            return tag != nullptr && tag->plane_type == HeightRangePlaneNodeTag::PlaneType::Max;
        },
        true
    );

    Scene::TriangleMesh& triangle_mesh_min =
        *m_triangle_mesh_manager.get_or_create(HeightRangePlaneNodeTag::PlaneType::Min, nullptr);
    Scene::TriangleMesh& triangle_mesh_max =
        *m_triangle_mesh_manager.get_or_create(HeightRangePlaneNodeTag::PlaneType::Max, nullptr);

    m_min_plane_node->set_raycast_component(
        new Scene::AabbRaycastNodeComponent(&triangle_mesh_min.aabb_mesh())
    );
    m_max_plane_node->set_raycast_component(
        new Scene::AabbRaycastNodeComponent(&triangle_mesh_max.aabb_mesh())
    );

    this->set_enabled(true);
    this->set_planes_visible(false);
}

void HeightRangePlanesWrapper::release(Scene::Scene& scene)
{
    if (!m_main_node) {
        return;
    }

    scene.remove_children(
        [](const Scene::Node* n) -> bool { return n->has_tag_of_type<HeightRangeNodeTag>(); },
        &scene.root()
    );

    m_geometry_manager.release(HeightRangePlaneNodeTag::PlaneType::Min);
    m_geometry_manager.release(HeightRangePlaneNodeTag::PlaneType::Max);
    m_triangle_mesh_manager.release(HeightRangePlaneNodeTag::PlaneType::Min);
    m_triangle_mesh_manager.release(HeightRangePlaneNodeTag::PlaneType::Max);

    m_main_node      = nullptr;
    m_min_plane_node = nullptr;
    m_max_plane_node = nullptr;
}

void HeightRangePlanesWrapper::set_enabled(const bool enabled)
{
    ASSERT(m_main_node != nullptr);
    m_main_node->set_enabled(enabled);
}

void HeightRangePlanesWrapper::set_planes_visible(const bool visible)
{
    ASSERT(m_min_plane_node != nullptr && m_max_plane_node != nullptr);
    m_min_plane_node->set_enabled(visible);
    m_max_plane_node->set_enabled(visible);
}

void HeightRangePlanesWrapper::set_positions(
    const double min_z,
    const double max_z,
    const ModelObject& model_object
)
{
    ASSERT(m_min_plane_node != nullptr && m_max_plane_node != nullptr);

    double visual_max_z = max_z;
    if (visual_max_z - min_z < HEIGHT_RANGE_PLANE_MIN_SEPARATION) {
        visual_max_z = min_z + HEIGHT_RANGE_PLANE_MIN_SEPARATION;
    }

    const BoundingBox3d& model_bb = Algorithms::ModelObject::bounding_box_approx(model_object);
    const Vec3d model_bb_center   = Algorithms::BoundingBox::center(model_bb);

    m_min_plane_node->set_local_transform(
        Domain::translation_transform(Vec3d(model_bb_center.x(), model_bb_center.y(), min_z))
    );
    m_max_plane_node->set_local_transform(
        Domain::translation_transform(Vec3d(model_bb_center.x(), model_bb_center.y(), visual_max_z))
    );
}

void HeightRangePlanesWrapper::set_plane_default_color(
    HeightRangePlaneNodeTag::PlaneType plane_type
)
{
    this->set_plane_color(this->plane_node(plane_type), HEIGHT_RANGE_PLANE_COLOR);
}

void HeightRangePlanesWrapper::set_plane_hover_color(HeightRangePlaneNodeTag::PlaneType plane_type)
{
    this->set_plane_color(this->plane_node(plane_type), HEIGHT_RANGE_PLANE_HOVER_COLOR);
}

void HeightRangePlanesWrapper::build_plane_node(
    Render::Device& device,
    Scene::NodeBuilder& builder,
    HeightRangePlaneNodeTag::PlaneType plane_type,
    const ModelObject& model_object
)
{
    const BoundingBox3d& model_bb = Algorithms::ModelObject::bounding_box_approx(model_object);
    const Vec3f model_bb_size     = Algorithms::BoundingBox::sizes(model_bb).cast<float>();
    const float plane_diameter    = model_bb_size.norm() * std::numbers::sqrt2_v<float> / 4.f;

    indexed_triangle_set plane_its;
    plane_its.vertices = {
        {plane_diameter, plane_diameter, 0.f},
        {-plane_diameter, plane_diameter, 0.f},
        {-plane_diameter, -plane_diameter, 0.f},
        {plane_diameter, -plane_diameter, 0.f},
    };
    plane_its.indices = {
        {0, 1, 2},
        {2, 3, 0},
    };

    const Scene::TriangleMesh& plane_triangle_mesh = *m_triangle_mesh_manager.get_or_create(
        plane_type,
        [&]() -> std::unique_ptr<Scene::TriangleMesh>
        { return std::make_unique<Scene::TriangleMesh>(std::move(plane_its)); }
    );
    const Render::Geometry& plane_geometry = *m_geometry_manager.get_or_create(
        plane_type,
        [&]()
        { return Render::geometry_from_triangle_mesh(device, plane_triangle_mesh.triangles()); }
    );
    const Render::Material material =
        Render::Material{}
            .set_shader(device.context().shader_manager().shader("gouraud_light"))
            .set_uniform("uniform_color", HEIGHT_RANGE_PLANE_COLOR)
            .set_transparent(true);

    builder.set_debug_name("HeightRangePlanes - Plane")
        .set_tag(HeightRangePlaneNodeTag(plane_type))
        .set_mesh(&plane_geometry, material, int(PlaterSceneLayer::DocumentObjects));
}

Scene::Node* HeightRangePlanesWrapper::plane_node(
    HeightRangePlaneNodeTag::PlaneType plane_type
) const
{
    return plane_type == HeightRangePlaneNodeTag::PlaneType::Min ? m_min_plane_node :
                                                                   m_max_plane_node;
}

void HeightRangePlanesWrapper::set_plane_color(Scene::Node* node, const ColorRGBA& color)
{
    Render::Material material = node->render_component()->material();
    material.set_uniform("uniform_color", color).set_transparent(color.is_transparent());
    node->set_material_override(material);
}

} // namespace Slic3r::App::Plater
