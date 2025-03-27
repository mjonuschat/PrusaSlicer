#include <span>

#include "Slic3r/Biz/Algorithms/Polyline.hpp"
#include "libpgcode/include/Slic3r/Biz/libpgcode/Types.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/PrePreview.hpp"

using namespace Slic3r::Biz;

namespace Slic3r::Biz::Print {

MoveVerticesPerLayer merge(MoveVerticesPerLayer&& a, MoveVerticesPerLayer&& b) {
    MoveVerticesPerLayer result{std::move(a)};

    for (auto &[layer_id, move_vertices] : b) {
        libpgcode::MoveVertices &result_layer{result[layer_id]};
        result_layer.insert(
            result_layer.end(),
            move_vertices.begin(),
            move_vertices.end()
        );
    }

    return result;
}

class WipeTowerHelper
{
public:
    WipeTowerHelper(const WipeTowerData& wipe_tower_data) : m_wipe_tower_data(wipe_tower_data) {
        if (wipe_tower_data.priming) {
            for (size_t i = 0; i < wipe_tower_data.priming.get()->size(); ++i) {
                m_priming.emplace_back(wipe_tower_data.priming.get()->at(i));
            }
        }
        if (wipe_tower_data.final_purge)
            m_final.emplace_back(*wipe_tower_data.final_purge.get());

        m_angle = wipe_tower_data.rotation_angle / 180.0f * PI;
        m_position = wipe_tower_data.position;
        m_layers_count = wipe_tower_data.tool_changes.size() + (m_priming.empty() ? 0 : 1);
    }

    const std::vector<Slic3r::WipeTower::ToolChangeResult>& tool_change(size_t idx) const {
        const auto& tool_changes = m_wipe_tower_data.tool_changes;
        return m_priming.empty() ?
            ((idx == tool_changes.size()) ? m_final : tool_changes[idx]) :
            ((idx == 0) ? m_priming : (idx == tool_changes.size() + 1) ? m_final : tool_changes[idx - 1]);
    }

    float get_angle() const { return m_angle; }
    const Slic3r::Vec2d& get_position() const { return m_position; }
    size_t get_layers_count() const { return m_layers_count; }

private:
    const WipeTowerData& m_wipe_tower_data;
    std::vector<Slic3r::WipeTower::ToolChangeResult> m_priming;
    std::vector<Slic3r::WipeTower::ToolChangeResult> m_final;
    Slic3r::Vec2d m_position{ Slic3r::Vec2d::Zero() };
    float m_angle{ 0.0f };
    size_t m_layers_count{ 0 };
};

libpgcode::MoveVertex create_move_vertex(
    const Vec3f& position,
    const float width,
    const float height,
    const libpgcode::MoveVertex& vertex_template
) {
    libpgcode::MoveVertex result{vertex_template};
    result.position = position;
    result.width = width;
    result.height = height;
    return result;
}

libpgcode::MoveVertices convert_lines_to_vertices(
    const Slic3r::Lines& lines,
    const std::vector<float>& widths,
    const std::vector<float>& heights,
    const float print_z,
    const libpgcode::MoveVertex& vertex_template
)
{
    if (lines.empty())
        return {};

    libpgcode::MoveVertices result;
    result.reserve(lines.size());

    for (size_t i{0}; i < lines.size(); ++i) {
        const Slic3r::Vec3f position{to_3d(unscaled(lines[i].b).cast<float>(), print_z)};
        result.push_back(create_move_vertex(position, widths[i], heights[i], vertex_template));
    }

    return result;
}

using ExtrusionSpan = std::span<const Slic3r::WipeTower::Extrusion>;
struct ExtrusionRange {
    ExtrusionSpan extrusions;
    unsigned tool;
};
using ExtrusionRanges = std::vector<ExtrusionRange>;

ExtrusionRanges get_extrusion_ranges(const ExtrusionSpan extrusions) {
    ExtrusionRanges result;

    std::optional<std::size_t> range_begin;
    std::optional<unsigned> current_tool;
    for (size_t i{}; i < extrusions.size(); ++i) {
        const Slic3r::WipeTower::Extrusion& extrusion{extrusions[i]};
        if (range_begin) {
            if (extrusion.width <= 0.0f || extrusion.tool != current_tool) {
                result.push_back({extrusions.subspan(*range_begin, i - *range_begin), *current_tool});
                range_begin = std::nullopt;
                current_tool = std::nullopt;
            }
        }
        if (!range_begin && extrusion.width > 0.0f) {
            range_begin = i;
            current_tool = extrusion.tool;
        }
    }
    if (range_begin) {
        result.push_back({extrusions.subspan(*range_begin, extrusions.size() - *range_begin), *current_tool});
    }

    return result;
}

libpgcode::MoveVertices convert_to_move_vertices(
    const ExtrusionRange& range,
    const std::function<Point(Point)>& transformation,
    const float layer_height,
    const float print_z
) {
    using Extrusion = Slic3r::WipeTower::Extrusion;

    if (range.extrusions.size() < 2) {
        return {};
    }

    Slic3r::Lines lines;
    std::vector<float> widths;
    lines.reserve(range.extrusions.size() - 1);
    widths.reserve(range.extrusions.size() - 1);

    Extrusion previous_extrusion{range.extrusions.front()};
    for (const Extrusion& extrusion : range.extrusions.subspan(1)) {
        const Point a{transformation(scaled(previous_extrusion.pos))};
        const Point b{transformation(scaled(extrusion.pos))};
        lines.push_back({a, b});
        widths.emplace_back(extrusion.width);
        previous_extrusion = extrusion;
    }

    const std::vector<float> heights(range.extrusions.size() - 1, layer_height);

    const libpgcode::MoveVertex vertex_template{
        .type = libpgcode::MoveType::Extrude,
        .extrusion_role = GCodeExtrusionRole::WipeTower,
        .extruder_id = static_cast<uint8_t>(range.tool)
    };

    libpgcode::MoveVertices result{convert_lines_to_vertices(
        lines, widths, heights, print_z, vertex_template
    )};

    return result;
}

MoveVerticesPerLayer get_wipe_tower_preview(const Slic3r::Print& print)
{
    if (!print.is_step_done(Slic3r::psWipeTower) || !print.has_wipe_tower()) {
        return {};
    }

    const WipeTowerData& wipe_tower_data{print.wipe_tower_data()};

    MoveVerticesPerLayer result{};
    const WipeTowerHelper wipe_tower_helper{wipe_tower_data};
    const float angle = wipe_tower_helper.get_angle();
    const Slic3r::Vec2d& position = wipe_tower_helper.get_position();

    for (size_t layer_id = 0; layer_id < wipe_tower_helper.get_layers_count(); ++layer_id) {
        const std::vector<Slic3r::WipeTower::ToolChangeResult>& tool_changes = wipe_tower_helper.tool_change(layer_id);
        for (const Slic3r::WipeTower::ToolChangeResult& tool_change : tool_changes) {
            for (const ExtrusionRange& range : get_extrusion_ranges(tool_change.extrusions)) {
                result[scaled(tool_change.print_z)] = convert_to_move_vertices(
                    range,
                    [&](const Point& point) {
                        return scaled(Vec2d{Eigen::Rotation2Dd(angle) * unscaled(point) + position});
                    },
                    tool_change.layer_height, tool_change.print_z
                );
            }
        }
    }

    return result;
}

libpgcode::MoveVertices path_to_vertices(
    const ExtrusionPath& path,
    const float print_z,
    const libpgcode::MoveVertex& vertex_template,
    const Point &offset = Point::Zero()
) {
    Slic3r::Polyline polyline = path.polyline;
    Algorithms::Polyline::remove_duplicate_points(polyline);
    polyline.translate(offset);
    const Slic3r::Lines lines = Algorithms::Polyline::to_lines(polyline);
    const std::vector<float> widths(lines.size(), path.width());
    const std::vector<float> heights(lines.size(), path.height());

    return convert_lines_to_vertices(lines, widths, heights, print_z, vertex_template);
}

template <typename ...Args>
libpgcode::MoveVertices convert_to_vertices(
    const ExtrusionLoop& loop,
    Args&&... args
) {
    libpgcode::MoveVertices result;
    for (const ExtrusionPath& path : loop.paths) {
        append(result, path_to_vertices(path, std::forward<Args>(args)...));
    }
    if (result.empty()){
        return {};
    }
    result.push_back(result.front());
    return result;
}

template <typename ...Args>
libpgcode::MoveVertices convert_to_vertices(
    const ExtrusionMultiPath& multi_path,
    Args&&... args
) {
    libpgcode::MoveVertices result;
    for (const ExtrusionPath& path : multi_path.paths) {
        append(result, path_to_vertices(path, std::forward<Args>(args)...));
    }
    return result;
}

template <typename ...Args>
libpgcode::MoveVertices convert_to_vertices(
    const ExtrusionEntityCollection& extrusion_entity_collection,
    Args&&... args
);

template <typename ...Args>
libpgcode::MoveVertices convert_entity_to_vertices(
    const Slic3r::ExtrusionEntity& extrusion_entity,
    Args&&... args
) {
    auto* extrusion_path{dynamic_cast<const Slic3r::ExtrusionPath*>(&extrusion_entity)};
    if (extrusion_path != nullptr) {
        return path_to_vertices(*extrusion_path, std::forward<Args>(args)...);
    }

    auto* extrusion_loop = dynamic_cast<const Slic3r::ExtrusionLoop*>(&extrusion_entity);
    if (extrusion_loop != nullptr) {
        return convert_to_vertices(*extrusion_loop, std::forward<Args>(args)...);
    }

    auto* extrusion_multi_path = dynamic_cast<const Slic3r::ExtrusionMultiPath*>(&extrusion_entity);
    if (extrusion_multi_path != nullptr) {
        return convert_to_vertices(*extrusion_multi_path, std::forward<Args>(args)...);
    }

    auto* extrusion_entity_collection = dynamic_cast<const Slic3r::ExtrusionEntityCollection*>(&extrusion_entity);
    if (extrusion_entity_collection != nullptr) {
        return convert_to_vertices(*extrusion_entity_collection, std::forward<Args>(args)...);
    }

    throw std::runtime_error{"Unknown entity type!"};
}

template <typename ...Args>
libpgcode::MoveVertices convert_to_vertices(
    const ExtrusionEntityCollection& collection,
    Args&&... args
)
{
    libpgcode::MoveVertices result;
    for (const Slic3r::ExtrusionEntity* entity : collection.entities) {
        if (entity == nullptr) {
            continue;
        }
        libpgcode::MoveVertices vertices{
            convert_entity_to_vertices(*entity, std::forward<Args>(args)...)
        };
        if (!vertices.empty()) {
            append(result, std::move(vertices));
            result.push_back(result.back());
            result.back().type = libpgcode::MoveType::Noop;
        }
    }
    return result;
}

std::vector<int> get_skirt_print_zs(const Slic3r::Print& print, std::vector<int>&& print_zs) {
    std::vector<int> result{std::move(print_zs)};

    if (print.has_infinite_skirt()) {
        return result;
    }
    const std::size_t skirt_height{std::max<size_t>(print.config().skirt_height.value, 0)};
    result.resize(std::min(skirt_height, result.size()));
    return result;
}

MoveVerticesPerLayer get_skirt_preview(
    const Slic3r::Print& print, std::vector<int>&& print_zs
)
{
    if (!print.is_step_done(psSkirtBrim)) {
        return {};
    }

    MoveVerticesPerLayer result;
    const std::vector<int>& skirt_print_zs{get_skirt_print_zs(print, std::move(print_zs))};

    for (const int print_z : skirt_print_zs) {
        const libpgcode::MoveVertex vertex_template{
            .type = libpgcode::MoveType::Extrude,
            .extrusion_role = GCodeExtrusionRole::Skirt,
            .extruder_id = static_cast<uint8_t>(0),
        };
        result[print_z] =
            convert_to_vertices(print.skirt(), unscaled(print_z), vertex_template);
    }
    return result;
}

MoveVerticesPerLayer get_brim_preview(const Slic3r::Print& print, const float height)
{
    if (!print.is_step_done(Slic3r::psSkirtBrim)) {
        return {};
    }

    const libpgcode::MoveVertex vertex_template{
        .type = libpgcode::MoveType::Extrude,
        .extrusion_role = GCodeExtrusionRole::Skirt,
        .extruder_id = static_cast<uint8_t>(0),
    };

    return {
        {scaled(height), convert_to_vertices(print.brim(), height, vertex_template)}
    };
}

void for_each_region(
    const PrintObject& object,
    const std::function<bool(const PrintObject&)>& skip_object,
    const std::function<void(const Layer&, const PrintInstance&, const LayerRegion&)>& func
)
{
    if (skip_object(object)) {
        return;
    }
    for (const Layer* layer : object.layers()) {
        for (const Slic3r::PrintInstance& instance : object.instances()) {
            for (const Slic3r::LayerRegion* region : layer->regions()) {
                func(*layer, instance, *region);
            }
        }
    }
}

MoveVerticesPerLayer get_perimeters_preview(
    const PrintObject& object
)
{
    MoveVerticesPerLayer result;

    for_each_region(
        object,
        [](const PrintObject& object) { return !object.is_step_done(Slic3r::posPerimeters); },
        [&](const Layer& layer, const PrintInstance& instance, const LayerRegion& region) {
            if (region.slices().empty())
                return;
            const Slic3r::PrintRegionConfig& config{region.region().config()};
            const auto extruder_id{
                static_cast<uint8_t>(std::max(config.perimeter_extruder.value - 1, 0))};

            const libpgcode::MoveVertex vertex_template{
                .type = libpgcode::MoveType::Extrude,
                .extrusion_role = GCodeExtrusionRole::ExternalPerimeter,
                .extruder_id = extruder_id};

            result[scaled(layer.print_z)] = convert_to_vertices(
                region.perimeters(), layer.print_z, vertex_template, instance.shift
            );
        }
    );
    return result;
}

MoveVerticesPerLayer get_infill_preview(
    const PrintObject& object
)
{
    MoveVerticesPerLayer result;

    for_each_region(
        object,
        [](const PrintObject& object) { return !object.is_step_done(Slic3r::posInfill); },
        [&](const Layer& layer, const PrintInstance& instance, const LayerRegion& region) {
            for (const Slic3r::ExtrusionEntity* entity : region.fills()) {
                const auto& fill{*dynamic_cast<const Slic3r::ExtrusionEntityCollection*>(entity)};
                if (fill.entities.empty()) {
                    continue;
                }

                const Slic3r::PrintRegionConfig& config{region.region().config()};
                const bool is_solid_infill = fill.entities.front()->role().is_solid_infill();
                const auto extruder_id = is_solid_infill ?
                    static_cast<uint8_t>(std::max(config.solid_infill_extruder.value - 1, 0)) :
                    static_cast<uint8_t>(std::max(config.infill_extruder.value - 1, 0));

                const libpgcode::MoveVertex vertex_template{
                    .type = libpgcode::MoveType::Extrude,
                    .extrusion_role =
                        is_solid_infill ?
                        GCodeExtrusionRole::SolidInfill :
                        GCodeExtrusionRole::InternalInfill,
                    .extruder_id = extruder_id
                };

                result[scaled(layer.print_z)] =
                    convert_to_vertices(fill, layer.print_z, vertex_template, instance.shift);
            }
        }
    );

    return result;
}

MoveVerticesPerLayer get_supports_preview(
    const PrintObject& object
)
{
    MoveVerticesPerLayer result;

    for_each_region(
        object,
        [](const PrintObject& object) { return !object.is_step_done(Slic3r::posSupportMaterial); },
        [&](const Layer& layer, const PrintInstance& instance, const LayerRegion& region) {
            const Slic3r::SupportLayer* support_layer{
                dynamic_cast<const Slic3r::SupportLayer*>(&layer)};
            if (support_layer == nullptr) {
                return;
            }

            const Slic3r::PrintObjectConfig& config{support_layer->object()->config()};
            for (const Slic3r::ExtrusionEntity* entity : support_layer->support_fills.entities) {
                const bool is_support_material = entity->role() ==
                    Slic3r::ExtrusionRole::SupportMaterial;
                const auto extruder_id{
                    is_support_material
                        ? static_cast<uint8_t>(
                              std::max(config.support_material_extruder.value - 1, 0)
                          )
                        : static_cast<uint8_t>(
                              std::max(config.support_material_interface_extruder.value - 1, 0)
                          )};

                const libpgcode::MoveVertex vertex_template{
                    .type = libpgcode::MoveType::Extrude,
                    .extrusion_role = is_support_material
                        ? GCodeExtrusionRole::SupportMaterial
                        : GCodeExtrusionRole::SupportMaterialInterface,
                    .extruder_id = extruder_id};

                result[scaled(layer.print_z)] = convert_to_vertices(
                    region.perimeters(), layer.print_z, vertex_template, instance.shift
                );
            }
        }
    );

    return result;
}

std::vector<Vec2f> double_to_float(const std::vector<Vec2d>& src)
{
    std::vector<Vec2f> ret;
    std::transform(src.begin(), src.end(), std::back_inserter(ret), [](const Vec2d& value) {
        return value.cast<float>();
    });
    return ret;
}

void Preview::update(MoveVerticesPerLayer&& moves) {
    std::scoped_lock lock{m_mutex};
    m_moves_per_layer = merge(std::move(m_moves_per_layer), std::move(moves));
}

libpgcode::ProcessorResult Preview::generate_result(const Slic3r::Print& print) const {
    std::scoped_lock lock{m_mutex};
    libpgcode::ProcessorResult result;

    std::size_t layer_id{0};
    for (const auto& [_, moves] : m_moves_per_layer) {
        for (const libpgcode::MoveVertex& move : moves) {
            result.moves.push_back(move);
            result.moves.back().layer_id = layer_id;
        }
        layer_id++;
    }

    result.producer = libpgcode::GCodeProducer::PrusaSlicer;
    result.extruders_count = static_cast<uint8_t>(print.config().nozzle_diameter.size());
    result.spiral_vase_enabled = print.config().spiral_vase;
    result.z_offset = print.config().z_offset;
    result.max_print_height = print.config().max_print_height;
    result.bed_shape = double_to_float(print.config().bed_shape.values);

    for (std::size_t gcode_id{}; gcode_id < result.moves.size(); ++gcode_id) {
        result.moves[gcode_id].gcode_id = gcode_id;
    }
    return result;
}

std::vector<int> Preview::get_scaled_print_zs() const {
    std::scoped_lock lock{m_mutex};

    std::vector<int> result;
    result.reserve(m_moves_per_layer.size());
    std::transform(m_moves_per_layer.begin(), m_moves_per_layer.end(),
       std::back_inserter(result),
       [](const auto& layer_moves) {
           return layer_moves.first; // scaled print_z
       }
    );
    return result;
}

libpgcode::ProcessorResult get_result_preview(const Slic3r::Print& print)
{
    Preview preview;
    for (const PrintObject* object : print.objects()) {
        preview.update(get_perimeters_preview(*object));
        preview.update(get_infill_preview(*object));
        preview.update(get_supports_preview(*object));
    }

    const auto brim_height{static_cast<float>(print.config().first_layer_height.value)};
    preview.update(get_brim_preview(print, brim_height));
    preview.update(get_wipe_tower_preview(print));
    preview.update(get_skirt_preview(print, preview.get_scaled_print_zs()));

    return preview.generate_result(print);
}

}
