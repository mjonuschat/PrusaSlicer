///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/CutGizmo.hpp"
#include "Slic3r/App/Plater/CutDialog.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/Domain/CutConnector.hpp"
#include "Slic3r/Domain/Constants.hpp"

#include <Slic3r/App/Render/GeometryBuilder.hpp>
#include "Slic3r/App/Scene/Node.hpp"
#include <Slic3r/App/Scene/NodeBuilder.hpp>
#include <Slic3r/App/Scene/NodeVisitor.hpp>
#include "Slic3r/App/Scene/MeshRenderNodeComponent.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"
#include "Slic3r/Biz/Algorithms/Geometry/Geometry.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Utils/CutUtils.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp" // ISceneSelectionChangedListener

#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/enum_bitmask.hpp"

#include "Slic3r/Math.hpp"

using namespace Slic3r::App::Yoga;

using Slic3r::Domain::ColorRGBA;

static const ColorRGBA UPPER_PART_COLOR    = ColorRGBA(0.0f, 1.0f, 1.0f, 1.0f);
static const ColorRGBA LOWER_PART_COLOR    = ColorRGBA(1.0f, 0.0f, 1.0f, 1.0f);
static const ColorRGBA CUT_PLANE_DEF_COLOR = ColorRGBA(0.9f, 0.9f, 0.9f, 0.5f);
static const ColorRGBA CUT_PLANE_ERR_COLOR = ColorRGBA(1.0f, 0.8f, 0.8f, 0.5f);

namespace Slic3r::App::Plater {

using namespace Slic3r::Domain;
using namespace Slic3r::Biz;
using namespace Biz::Algorithms::Geometry;

CutGizmo::CutGizmo(
    Render::Device& device,
    PlaterScenePresenter& scene_presenter,
    Biz::ProjectInteractor* project_interactor
) :
    m_device(device),
    m_scene_presenter(scene_presenter),
    m_project_interactor(project_interactor)
{
    m_dialog                      = std::make_unique<CutDialog>();
    m_dialog->callbacks().perform = [this]() { perform_cut(); };

    m_dialog->callbacks().z_changed = [this](double new_Z)
    {
        m_plane_center.z() = new_Z;
        update_cut_plane_trafo();

        process_contours();
    };

    m_dialog->callbacks().mode_changed = [this]()
    {
        update_cut_plane_mesh();
        update_cut_plane_trafo();
    };

    m_dialog->callbacks().groove_depth_value_changed = [this](double value)
    {
        m_groove.depth = value;
        update_cut_plane_mesh();
    };
    m_dialog->callbacks().groove_depth_tolerance_changed = [this](double value)
    {
        m_groove.depth_tolerance = value;
        update_cut_plane_mesh();
    };
    m_dialog->callbacks().groove_width_value_changed = [this](double value)
    {
        m_groove.width = value;
        update_cut_plane_mesh();
    };
    m_dialog->callbacks().groove_width_tolerance_changed = [this](double value)
    {
        m_groove.width_tolerance = value;
        update_cut_plane_mesh();
    };
    m_dialog->callbacks().flap_angle_changed = [this](double value)
    {
        // Convert the degree value to an angle in radians.
        m_groove.flaps_angle = deg2rad(value);
        update_cut_plane_mesh();
    };
    m_dialog->callbacks().groove_angle_changed = [this](double value)
    {
        // Convert the degree value to an angle in radians.
        m_groove.angle = deg2rad(value);
        update_cut_plane_mesh();
    };
}

void CutGizmo::on_activated()
{
    init_scene_nodes();

    m_dialog->set_current_connetor_shape(Domain::CutConnectorShape::Circle);
    m_dialog->set_current_connetor_style(Domain::CutConnectorStyle::Prism);
    m_dialog->set_current_connetor_type(Domain::CutConnectorType::Snap);

    update_scene_nodes();
    set_enabled_scene_nodes(false);
}

void CutGizmo::on_deactivated()
{
    reset();
    set_enabled_scene_nodes(true);
}

Scene::ToolType CutGizmo::type() const
{
    return Scene::ToolType::CutGizmo;
}

Yoga::GizmoDialog* CutGizmo::ui_dialog()
{
    return m_dialog.get();
}

Scene::GizmoActivationState CutGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    return Scene::GizmoActivationState();
}

// code is borrowed from:
// #include <arrange-wrapper/SceneBuilder.hpp>

Domain::BoundingBox3d instance_bounding_box(
    const Domain::ModelInstance& mi,
    const Domain::Transform3d& tr = Domain::Transform3d::Identity(),
    bool dont_translate           = false
)
{
    using Slic3r::Biz::Algorithms::BoundingBox::merge;

    Domain::BoundingBox3d bb;
    const Domain::Transform3d inst_matrix = dont_translate ?
        mi.get_transformation().get_matrix_no_offset() :
        mi.get_transformation().get_matrix();

    for (Domain::ModelVolume* v : mi.get_object()->volumes) {
        if (v->is_model_part()) {
            bb = merge(
                bb,
                Slic3r::Biz::Algorithms::ModelVolume::transformed_bounding_box(
                    *v,
                    tr * inst_matrix * v->get_matrix()
                )
            );
        }
    }

    return bb;
}

static indexed_triangle_set its_make_groove_plane(
    Biz::Cut::Groove& groove,
    std::vector<Domain::Vec3d>& groove_vertices,
    double radius
)
{
    // values for calculation

    const float side_width  = static_cast<float>(is_approx(groove.flaps_angle, 0.) ?
         groove.depth :
         (groove.depth / sin(groove.flaps_angle)));
    const float flaps_width = 2.f * side_width * static_cast<float>(cos(groove.flaps_angle));

    const float groove_half_width_upper = 0.5f * static_cast<float>(groove.width);
    const float groove_half_width_lower = 0.5f * (static_cast<float>(groove.width) + flaps_width);

    const float cut_plane_radius = 1.5f * static_cast<float>(radius);
    const float cut_plane_length = 1.5f * cut_plane_radius;

    const float groove_half_depth = 0.5f * static_cast<float>(groove.depth);

    const float x = 0.5f * cut_plane_radius;
    const float y = 0.5f * cut_plane_length;
    float z_upper = groove_half_depth;
    float z_lower = -groove_half_depth;

    const float proj = y * static_cast<float>(tan(groove.angle));

    float ext_upper_x = groove_half_width_upper + proj; // upper_x extension
    float ext_lower_x = groove_half_width_lower + proj; // lower_x extension

    float nar_upper_x = groove_half_width_upper - proj; // upper_x narrowing
    float nar_lower_x = groove_half_width_lower - proj; // lower_x narrowing

    const float cut_plane_thiknes =
        0.02f; // 0.02f * (float)get_grabber_mean_size(m_bounding_box);   // cut_plane_thiknes

    // Vertices of the groove used to detection if groove is valid
    // They are written as:
    // {left_ext_lower, left_nar_lower, left_ext_upper, left_nar_upper,
    // right_ext_lower, right_nar_lower, right_ext_upper, right_nar_upper }
    {
        groove_vertices.clear();
        groove_vertices.reserve(8);

        groove_vertices.emplace_back(Vec3f(-ext_lower_x, -y, z_lower).cast<double>());
        groove_vertices.emplace_back(Vec3f(-nar_lower_x, y, z_lower).cast<double>());
        groove_vertices.emplace_back(Vec3f(-ext_upper_x, -y, z_upper).cast<double>());
        groove_vertices.emplace_back(Vec3f(-nar_upper_x, y, z_upper).cast<double>());
        groove_vertices.emplace_back(Vec3f(ext_lower_x, -y, z_lower).cast<double>());
        groove_vertices.emplace_back(Vec3f(nar_lower_x, y, z_lower).cast<double>());
        groove_vertices.emplace_back(Vec3f(ext_upper_x, -y, z_upper).cast<double>());
        groove_vertices.emplace_back(Vec3f(nar_upper_x, y, z_upper).cast<double>());
    }

    // Different cases of groove plane:

    // groove is open

    if (groove_half_width_upper > proj && groove_half_width_lower > proj) {
        indexed_triangle_set mesh;

        auto get_vertices = [x,
                             y](float z_upper,
                                float z_lower,
                                float nar_upper_x,
                                float nar_lower_x,
                                float ext_upper_x,
                                float ext_lower_x)
        {
            return std::vector<stl_vertex>(
                {// upper left part vertices
                 {-x, -y, z_upper},
                 {-x, y, z_upper},
                 {-nar_upper_x, y, z_upper},
                 {-ext_upper_x, -y, z_upper},
                 // lower part vertices
                 {-ext_lower_x, -y, z_lower},
                 {-nar_lower_x, y, z_lower},
                 {nar_lower_x, y, z_lower},
                 {ext_lower_x, -y, z_lower},
                 // upper right part vertices
                 {ext_upper_x, -y, z_upper},
                 {nar_upper_x, y, z_upper},
                 {x, y, z_upper},
                 {x, -y, z_upper}
                }
            );
        };

        mesh.vertices =
            get_vertices(z_upper, z_lower, nar_upper_x, nar_lower_x, ext_upper_x, ext_lower_x);
        mesh.vertices.reserve(2 * mesh.vertices.size());

        z_upper -= cut_plane_thiknes;
        z_lower -= cut_plane_thiknes;

        const float under_x_shift = cut_plane_thiknes / tan(0.5f * groove.flaps_angle);

        nar_upper_x += under_x_shift;
        nar_lower_x += under_x_shift;
        ext_upper_x += under_x_shift;
        ext_lower_x += under_x_shift;

        std::vector<stl_vertex> vertices =
            get_vertices(z_upper, z_lower, nar_upper_x, nar_lower_x, ext_upper_x, ext_lower_x);
        mesh.vertices.insert(mesh.vertices.end(), vertices.begin(), vertices.end());

        mesh.indices = {
            // above view
            {5, 4, 7},
            {5, 7, 6}, // lower part
            {3, 4, 5},
            {3, 5, 2}, // left side
            {9, 6, 8},
            {8, 6, 7}, // right side
            {1, 0, 2},
            {2, 0, 3}, // upper left part
            {9, 8, 10},
            {10, 8, 11}, // upper right part
            // under view
            {20, 21, 22},
            {20, 22, 23}, // upper right part
            {12, 13, 14},
            {12, 14, 15}, // upper left part
            {18, 21, 20},
            {18, 20, 19}, // right side
            {16, 15, 14},
            {16, 14, 17}, // left side
            {16, 17, 18},
            {16, 18, 19}, // lower part
            // left edge
            {1, 13, 0},
            {0, 13, 12},
            // front edge
            {0, 12, 3},
            {3, 12, 15},
            {3, 15, 4},
            {4, 15, 16},
            {4, 16, 7},
            {7, 16, 19},
            {7, 19, 20},
            {7, 20, 8},
            {8, 20, 11},
            {11, 20, 23},
            // right edge
            {11, 23, 10},
            {10, 23, 22},
            // back edge
            {1, 13, 2},
            {2, 13, 14},
            {2, 14, 17},
            {2, 17, 5},
            {5, 17, 6},
            {6, 17, 18},
            {6, 18, 9},
            {9, 18, 21},
            {9, 21, 10},
            {10, 21, 22}
        };
        return mesh;
    }

    const float tan_groove_angle = static_cast<float>(tan(groove.angle));
    float cross_pt_upper_y = groove_half_width_upper / tan_groove_angle;

    // groove is closed

    const float tan_half_flaps_angle = static_cast<float>(tan(0.5f * groove.flaps_angle));
    if (groove_half_width_upper < proj && groove_half_width_lower < proj) {
        float cross_pt_lower_y = groove_half_width_lower / tan_groove_angle;

        indexed_triangle_set mesh;

        auto get_vertices = [x,
                             y](float z_upper,
                                float z_lower,
                                float cross_pt_upper_y,
                                float cross_pt_lower_y,
                                float ext_upper_x,
                                float ext_lower_x)
        {
            return std::vector<stl_vertex>(
                {// upper part vertices
                 {-x, -y, z_upper},
                 {-x, y, z_upper},
                 {x, y, z_upper},
                 {x, -y, z_upper},
                 {ext_upper_x, -y, z_upper},
                 {0.f, cross_pt_upper_y, z_upper},
                 {-ext_upper_x, -y, z_upper},
                 // lower part vertices
                 {-ext_lower_x, -y, z_lower},
                 {0.f, cross_pt_lower_y, z_lower},
                 {ext_lower_x, -y, z_lower}
                }
            );
        };

        mesh.vertices = get_vertices(
            z_upper,
            z_lower,
            cross_pt_upper_y,
            cross_pt_lower_y,
            ext_upper_x,
            ext_lower_x
        );
        mesh.vertices.reserve(2 * mesh.vertices.size());

        z_upper -= cut_plane_thiknes;
        z_lower -= cut_plane_thiknes;

        const float under_x_shift = cut_plane_thiknes / tan_half_flaps_angle;

        cross_pt_upper_y += cut_plane_thiknes;
        cross_pt_lower_y += cut_plane_thiknes;
        ext_upper_x += under_x_shift;
        ext_lower_x += under_x_shift;

        std::vector<stl_vertex> vertices = get_vertices(
            z_upper,
            z_lower,
            cross_pt_upper_y,
            cross_pt_lower_y,
            ext_upper_x,
            ext_lower_x
        );
        mesh.vertices.insert(mesh.vertices.end(), vertices.begin(), vertices.end());

        mesh.indices = {
            // above view
            {8, 7, 9}, // lower part
            {5, 8, 6},
            {6, 8, 7}, // left side
            {4, 9, 8},
            {4, 8, 5}, // right side
            {1, 0, 6},
            {1, 6, 5},
            {1, 5, 2},
            {2, 5, 4},
            {2, 4, 3}, // upper part
            // under view
            {10, 11, 16},
            {16, 11, 15},
            {15, 11, 12},
            {15, 12, 14},
            {14, 12, 13}, // upper part
            {18, 15, 14},
            {14, 18, 19}, // right side
            {17, 16, 15},
            {17, 15, 18}, // left side
            {17, 18, 19}, // lower part
            // left edge
            {1, 11, 0},
            {0, 11, 10},
            // front edge
            {0, 10, 6},
            {6, 10, 16},
            {6, 17, 16},
            {6, 7, 17},
            {7, 17, 19},
            {7, 19, 9},
            {4, 14, 19},
            {4, 19, 9},
            {4, 14, 13},
            {4, 13, 3},
            // right edge
            {3, 13, 12},
            {3, 12, 2},
            // back edge
            {2, 12, 11},
            {2, 11, 1}
        };

        return mesh;
    }

    // groove is closed from the roof

    indexed_triangle_set mesh;
    mesh.vertices = {
        // upper part vertices
        {-x, -y, z_upper},
        {-x, y, z_upper},
        {x, y, z_upper},
        {x, -y, z_upper},
        {ext_upper_x, -y, z_upper},
        {0.f, cross_pt_upper_y, z_upper},
        {-ext_upper_x, -y, z_upper},
        // lower part vertices
        {-ext_lower_x, -y, z_lower},
        {-nar_lower_x, y, z_lower},
        {nar_lower_x, y, z_lower},
        {ext_lower_x, -y, z_lower}
    };

    mesh.vertices.reserve(2 * mesh.vertices.size() + 1);

    z_upper -= cut_plane_thiknes;
    z_lower -= cut_plane_thiknes;

    const float under_x_shift = cut_plane_thiknes / tan_half_flaps_angle;

    nar_lower_x += under_x_shift;
    ext_upper_x += under_x_shift;
    ext_lower_x += under_x_shift;

    std::vector<stl_vertex> vertices = {
        // upper part vertices
        {-x, -y, z_upper},
        {-x, y, z_upper},
        {x, y, z_upper},
        {x, -y, z_upper},
        {ext_upper_x, -y, z_upper},
        {under_x_shift, cross_pt_upper_y, z_upper},
        {-under_x_shift, cross_pt_upper_y, z_upper},
        {-ext_upper_x, -y, z_upper},
        // lower part vertices
        {-ext_lower_x, -y, z_lower},
        {-nar_lower_x, y, z_lower},
        {nar_lower_x, y, z_lower},
        {ext_lower_x, -y, z_lower}
    };
    mesh.vertices.insert(mesh.vertices.end(), vertices.begin(), vertices.end());

    mesh.indices = {
        // above view
        {8, 7, 10},
        {8, 10, 9}, // lower part
        {5, 8, 7},
        {5, 7, 6}, // left side
        {4, 10, 9},
        {4, 9, 5}, // right side
        {1, 0, 6},
        {1, 6, 5},
        {1, 5, 2},
        {2, 5, 4},
        {2, 4, 3}, // upper part
        // under view
        {11, 12, 18},
        {18, 12, 17},
        {17, 12, 16},
        {16, 12, 13},
        {16, 13, 15},
        {15, 13, 14}, // upper part
        {21, 16, 15},
        {21, 15, 22}, // right side
        {19, 18, 17},
        {19, 17, 20}, // left side
        {19, 20, 21},
        {19, 21, 22}, // lower part
        // left edge
        {1, 12, 11},
        {1, 11, 0},
        // front edge
        {0, 11, 18},
        {0, 18, 6},
        {7, 19, 18},
        {7, 18, 6},
        {7, 19, 22},
        {7, 22, 10},
        {10, 22, 15},
        {10, 15, 4},
        {4, 15, 14},
        {4, 14, 3},
        // right edge
        {3, 14, 13},
        {3, 14, 2},
        // back edge
        {2, 13, 12},
        {2, 12, 1},
        {5, 16, 21},
        {5, 21, 9},
        {9, 21, 20},
        {9, 20, 8},
        {5, 17, 20},
        {5, 20, 8}
    };

    return mesh;
}

static double get_grabber_mean_size(Domain::Vec3d bb_size)
{
    return (bb_size.x() + bb_size.y() + bb_size.z()) / 30.;
}

void CutGizmo::on_scene_selection_changed(
    Domain::SelectionId project_id,
    const Biz::Scene::ObjectSelection& selection
)
{}

void CutGizmo::update_scene_nodes()
{
    static const double in_to_mm = 25.4;
    static const double mm_to_in = 1 / in_to_mm;

    const Biz::Scene::ObjectSelection& selection =
        m_project_interactor->scene_interactor().object_selection();

    if (selection.empty() || selection.mode != Slic3r::Biz::Scene::SelectionMode::Instance) {
        // on_deactivated();

        // We can’t perform a cut for multiple objects simultaneously.
        return;
    }

    // update cut_normal
    Vec3d normal = m_rotation_m * Vec3d::UnitZ();
    normal.normalize();
    m_cut_normal = normal;

    Domain::Project& project = m_project_interactor->selected_project();

    for (const Domain::ElementRef& element : selection.elements) {
        assert(element.volume_id == 0); // is object
        m_selected_object     = project.find_object_by_id(element.object_id);
        m_selected_instance   = project.find_instance_by_id(element.object_id, element.instance_id);
        m_transformed_bbox    = instance_bounding_box(*m_selected_instance);
        Domain::Vec3d bb_size = m_transformed_bbox.max - m_transformed_bbox.min;
        m_mean_size =
            (bb_size.x() + bb_size.y() + bb_size.z()) / 9.0 * (m_imperial_units ? mm_to_in : 1.);

        m_plane_center = (m_transformed_bbox.min + m_transformed_bbox.max) * 0.5;

        // get instance index
        m_instance_idx = 0;
        for (const auto* inst : m_selected_object->instances) {
            if (inst == m_selected_instance)
                break;
            m_instance_idx++;
        }
        ASSERT(m_instance_idx < m_selected_object->instances.size());

        m_radius = 0.5 * bb_size.norm();

        m_groove.depth = m_groove.depth_init =
            std::max(1.f, 0.5f * float(get_grabber_mean_size(bb_size)));
        m_groove.width = m_groove.width_init = 4.0f * m_groove.depth;
        m_groove.flaps_angle                 = m_groove.flaps_angle_init =
            float(Biz::Algorithms::Geometry::PI) / 3.f;
        m_groove.angle = m_groove.angle_init = 0.f;

        m_dialog->set_build_size(bb_size);
        m_dialog->set_groove_values(m_groove, m_mean_size);

        update_cut_plane_mesh();
        update_cut_plane_trafo();
    }
}

static const std::unordered_map<CutMeshType, ColorRGBA> MESH_COLORS = {
    {CutMeshType::UpperPart, {1.f, 0.5f, 0.f, 1.f}},
    {CutMeshType::LowerPart, {0.5f, 0.5f, 0.5f, 1.f}},
    {CutMeshType::Plane, {0.6f, 0.2f, 1.0f, 0.21f}},
};

void CutGizmo::build_cut_part_mesh(
    CutMeshType type,
    size_t part_id,
    std::shared_ptr<const Slic3r::Domain::TriangleMesh> mesh,
    const Transform3d& trafo,
    Scene::NodeBuilder& builder
)
{
    const std::string type_str = type == CutMeshType::UpperPart ? "UpperPart" :
        type == CutMeshType::LowerPart                          ? "LowerPart" :
                                                                  "UNDEF";
    SPDLOG_DEBUG("build_volume type:{}", type_str);

    auto& geom_mgr    = m_model_geometry_manager;
    auto& trimesh_mgr = m_model_triangle_mesh_manager;

    Scene::AuxiliaryElementId::Type aei_type = type == CutMeshType::UpperPart ?
        Scene::AuxiliaryElementId::Type::CutUpperPart :
        Scene::AuxiliaryElementId::Type::CutLowerPart;

    Scene::AuxiliaryElementId id{aei_type, part_id};
    const auto& trimesh = trimesh_mgr.get_or_create(
        id,
        [&]() -> std::unique_ptr<Scene::TriangleMesh>
        { return std::make_unique<Scene::TriangleMesh>(mesh); }
    );
    const auto* geom = geom_mgr.get_or_create(
        id,
        [&]() { return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles()); }
    );
    ColorRGBA color = type == CutMeshType::UpperPart ? UPPER_PART_COLOR :
        type == CutMeshType::LowerPart               ? LOWER_PART_COLOR :
                                                       ColorRGBA{1.0f, 1.0f, 1.0f, 1.0f};

    auto material = Render::Material{}
                        .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
                        .set_uniform("uniform_color", color)
                        .set_transparent(color.is_transparent());

    builder.set_debug_name(fmt::format("cut type: {}", type_str))
        .set_tag(CutNodeTag{type, part_id})
        .set_mesh(geom, material, int(0))
        .transform([trafo](auto& xform) { xform = trafo; })
        .set_aabb(trimesh->aabb_mesh())
        .set_shadows(Render::Shadows{true, true})
        .set_pbr(Scene::DEFAULT_VOLUME_PBRPARAMS);
}

void CutGizmo::reset_cut_part_meshes()
{
    // Remove all cut parts Scene::Nodes
    std::vector<Scene::AuxiliaryElementId> geometry_ids;
    m_scene_presenter.scene().remove_children(
        [&](const Scene::Node* node)
        {
            const CutNodeTag* t = node->tag_of_type<CutNodeTag>();
            bool ret            = t != nullptr;
            if (ret) {
                Scene::AuxiliaryElementId id;
                if (t->type == CutMeshType::UpperPart) {
                    id.type = Scene::AuxiliaryElementId::Type::CutUpperPart;
                } else if (t->type == CutMeshType::LowerPart) {
                    id.type = Scene::AuxiliaryElementId::Type::CutLowerPart;
                } else {
                    return false;
                }
                id.id = t->cut_part_id;
                geometry_ids.push_back(id);
            }
            return ret;
        },
        m_main_node
    );

    for (const Scene::AuxiliaryElementId& id : geometry_ids) {
        m_model_geometry_manager.release(id);
        m_model_triangle_mesh_manager.release(id);
    }
}

void CutGizmo::reset()
{
//    reset_cut_part_meshes();

    // Remove all cut parts Scene::Nodes
    std::vector<Scene::AuxiliaryElementId> geometry_ids;
    m_scene_presenter.scene().remove_children(
        [&](const Scene::Node* node)
        {
            const CutNodeTag* t = node->tag_of_type<CutNodeTag>();
            bool ret = t != nullptr;
            if (ret) {
                Scene::AuxiliaryElementId id;
                id.type = t->type == CutMeshType::UpperPart ? Scene::AuxiliaryElementId::Type::CutUpperPart :
                    t->type == CutMeshType::LowerPart ? Scene::AuxiliaryElementId::Type::CutLowerPart :
                    t->type == CutMeshType::Connector ? Scene::AuxiliaryElementId::Type::CutConnector :
                    t->type == CutMeshType::Clip ? Scene::AuxiliaryElementId::Type::CutClip : Scene::AuxiliaryElementId::Type::CutPlane;
                id.id = t->type == CutMeshType::UpperPart || t->type == CutMeshType::LowerPart ? t->cut_part_id :
                    t->type == CutMeshType::Connector ? t->connector_id : 0;
                geometry_ids.push_back(id);
            }
            return ret;
        },
        m_main_node
    );

    for (const Scene::AuxiliaryElementId& id : geometry_ids) {
        m_model_geometry_manager.release(id);
        m_model_triangle_mesh_manager.release(id);
    }
}

void CutGizmo::set_enabled_scene_nodes(bool enabled)
{
    Scene::visit(
        m_scene_presenter.scene().root(),
        [&](Scene::Node& node)
        {
            SceneNodeTag* tag = node.tag_of_type<SceneNodeTag>();
            if (tag != nullptr) {
                node.set_enabled(enabled);
            }
        }
    );
}

void CutGizmo::build_cut_plane_node(Scene::NodeBuilder& builder)
{
    SPDLOG_DEBUG("build_volume type:Cut plane");

    // Make default mesh as small as possible.
    // Its geometry will be updated on layers move
    indexed_triangle_set mesh_its = Biz::Algorithms::TriangleMesh::its_make_cube(0.1f, 0.1f, 0.1f);

    Scene::AuxiliaryElementId id{Scene::AuxiliaryElementId::Type::CutPlane};

    const auto& trimesh = m_model_triangle_mesh_manager.get_or_create(
        id,
        [&]() -> std::unique_ptr<Scene::TriangleMesh>
        { return std::make_unique<Scene::TriangleMesh>(std::move(mesh_its)); }
    );
    const auto* geom = m_model_geometry_manager.get_or_create(
        id,
        [&]() { return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles()); }
    );

    ColorRGBA color = CUT_PLANE_DEF_COLOR;
    auto material   = Render::Material{}
                        .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
                        .set_uniform("uniform_color", color)
        // .set_transparent(color.is_transparent())
        ;

    builder.set_debug_name("cut: cut plane:")
        .set_tag(CutNodeTag{CutMeshType::Plane})
        .set_mesh(geom, material, int(0))
        .set_shadows(Render::Shadows{false, false})
        .set_pbr(Scene::DEFAULT_VOLUME_PBRPARAMS);
    ;
}

void CutGizmo::update_cut_plane_mesh()
{
    const double cp_width    = 0.02 * 10.; // get_grabber_mean_size(m_bounding_box);
    indexed_triangle_set its = is_planar_mode() ?
        Biz::Algorithms::TriangleMesh::its_make_frustum_dowel(1.2 * m_radius, cp_width, 4) :
        its_make_groove_plane(m_groove, m_groove_vertices, m_radius);

    Scene::visit(
        *m_main_node,
        [&](Scene::Node& n)
        {
            CutNodeTag* tag = n.tag_of_type<CutNodeTag>();
            if (tag != nullptr && tag->type == CutMeshType::Plane) {
                Scene::AuxiliaryElementId id{Scene::AuxiliaryElementId::Type::CutPlane};

                m_model_triangle_mesh_manager.release(id);
                m_model_geometry_manager.release(id);

                const auto& trimesh = m_model_triangle_mesh_manager.get_or_create(
                    id,
                    [&, this]() -> std::unique_ptr<Scene::TriangleMesh>
                    { return std::make_unique<Scene::TriangleMesh>(std::move(its)); }
                );
                const auto* geom = m_model_geometry_manager.get_or_create(
                    id,
                    [&]()
                    { return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles()); }
                );

                static_cast<Scene::MeshRenderNodeComponent*>(n.render_component())
                    ->set_geometry(geom);
            }
        }
    );

    // reset_cut_part_meshes();
    process_contours();
}

void CutGizmo::update_cut_plane_trafo()
{
    Scene::visit(
        *m_main_node,
        [&](Scene::Node& node)
        {
            CutNodeTag* tag = node.tag_of_type<CutNodeTag>();
            if (tag != nullptr && tag->type == CutMeshType::Plane) {
                Render::Material material = node.render_component()->material();
                ColorRGBA cp_clr = can_perform_cut() && has_valid_groove() ? CUT_PLANE_DEF_COLOR :
                                                                             CUT_PLANE_ERR_COLOR;
                material.set_uniform("uniform_color", cp_clr)
                    .set_transparent(cp_clr.is_transparent());
                node.set_material_override(material);

                node.set_local_transform(
                    Domain::translation_transform(m_plane_center) * m_rotation_m
                );
            }
        }
    );
}

void CutGizmo::build_clipping_plane_node(Scene::NodeBuilder& builder)
{
    SPDLOG_DEBUG("build_volume type:Clip");

    // Make default mesh as small as possible.
    // Its geometry will be updated on layers move
    indexed_triangle_set mesh_its = Biz::Algorithms::TriangleMesh::its_make_cube(0.1f, 0.1f, 0.1f);
    Scene::AuxiliaryElementId id{Scene::AuxiliaryElementId::Type::CutClip};

    const auto& trimesh = m_model_triangle_mesh_manager.get_or_create(
        id,
        [&]() -> std::unique_ptr<Scene::TriangleMesh>
        { return std::make_unique<Scene::TriangleMesh>(std::move(mesh_its)); }
    );
    const auto* geom = m_model_geometry_manager.get_or_create(
        id,
        [&]() { return Render::geometry_from_triangle_mesh(m_device, trimesh->triangles()); }
    );

    ColorRGBA color = ColorRGBA{1.0f, 0.0f, 0.37f, 1.0f};
    auto material   = Render::Material{}
                        .set_shader(m_device.context().shader_manager().shader("gouraud_light"))
                        .set_uniform("uniform_color", color)
                        .set_transparent(color.is_transparent());

    builder.set_debug_name("cut: clipping plane:")
        .set_tag(CutNodeTag{CutMeshType::Clip})
        .set_mesh(geom, material, int(0))
        .set_shadows(Render::Shadows{true, true});
}

void CutGizmo::init_scene_nodes()
{
    /* Create a tree
     * -> "cut_sla"             {CutNodeTag{   SlaMeshType::Undefined }}
     *       -> "upper_part_mesh"      {CutNodeTag{   SlaMeshType::UpperPart }}
     *       -> "lower_part_mesh"      {CutNodeTag{   SlaMeshType::LowerPart }}
     *       -> "clip_mesh"      {CutNodeTag{   SlaMeshType::Clip }}
     *       -> "groove_mesh"      {CutNodeTag{   SlaMeshType::Groove }}
     *       -> "plane_mesh"      {CutNodeTag{   SlaMeshType::Plane }}
     *       -> "connectors_node"    {CutNodeTag{ SlaMeshType::Connector, connector_id=-1 }}
     *               -> "connector_mesh"           {CutNodeTag{ SlaMeshType::Connector, connector_id }}
     */

    Scene::Scene& scene = m_scene_presenter.scene();
    Scene::Node* node   = scene.root().query_first(
        [](const Scene::Node* n) -> bool
        {
            const CutNodeTag* tag = n->tag_of_type<CutNodeTag>();
            return tag != nullptr && tag->type == CutMeshType::Undefined;
        },
        true
    );

    if (node != nullptr) {
        m_main_node = node;
//        return;
    }

    Scene::NodeBuilder builder{scene};
    builder.set_debug_name("cut_main");
    builder.set_tag(CutNodeTag());

    builder.child([&](Scene::NodeBuilder& bldr) { build_cut_plane_node(bldr); });

    builder.child([&](Scene::NodeBuilder& bldr) { build_clipping_plane_node(bldr); });

    scene.add_child(builder.build().release(), &scene.root());
    m_main_node = scene.root().children().back().get();
}

Domain::Transform3d CutGizmo::get_cut_matrix()
{
    if (!m_selected_instance)
        return Domain::Transform3d::Identity();

    // m_cut_z is the distance from the bed. Subtract possible SLA elevation.
    const double sla_shift_z = 0.; // selection.get_first_volume()->get_sla_shift_z();

    const Domain::Vec3d instance_offset = m_selected_instance->get_offset();
    Domain::Vec3d cut_center_offset     = m_plane_center - instance_offset;
    cut_center_offset.z() -= sla_shift_z;

    return Domain::translation_transform(cut_center_offset) * m_rotation_m;
}

bool CutGizmo::can_perform_cut() const
{
    return true;
}

bool CutGizmo::has_valid_groove() const
{
    if (is_planar_mode())
        return true;

    const float flaps_width = -2.f * m_groove.depth / tan(m_groove.flaps_angle);
    if (flaps_width > m_groove.width)
        return false;
    /*
        const Selection& selection = m_parent.get_selection();
        const auto& list = selection.get_volume_idxs();
        // is more volumes selected?
        if (list.empty())
            return false;

        const Transform3d cp_matrix = translation_transform(m_plane_center) * m_rotation_m;

        for (size_t id = 0; id < m_groove_vertices.size(); id += 2) {
            const Vec3d beg = cp_matrix * m_groove_vertices[id];
            const Vec3d end = cp_matrix * m_groove_vertices[id + 1];

            bool intersection = false;
            for (const unsigned int volume_idx : list) {
                const GLVolume* glvol = selection.get_volume(volume_idx);
                if (!glvol->is_modifier &&
                    glvol->mesh_raycaster->intersects_line(beg, end - beg, glvol->world_matrix())) {
                    intersection = true;
                    break;
                }
            }
            if (!intersection)
                return false;
        }
    */
    return true;
}

void CutGizmo::apply_connectors_in_model(ModelObject* mo, int& dowels_count)
{
    if (is_planar_mode()) {
        // clear_selection();

        for (CutConnector& connector : mo->cut_connectors) {
            connector.rotation_m = m_rotation_m;

            if (connector.attribs.type == CutConnectorType::Dowel) {
                if (connector.attribs.style == CutConnectorStyle::Prism)
                    connector.height *= 2;
                dowels_count++;
            } else {
                // calculate shift of the connector center regarding to the position on the cut plane
                connector.pos += m_cut_normal * 0.5 * double(connector.height);
            }
        }
        apply_cut_connectors(mo, _u8L("Connector"));
    }
}

static indexed_triangle_set get_connector_mesh(
    CutConnectorAttributes connector_attributes,
    float snap_bulge_proportion,
    float snap_space_proportion
)
{
    indexed_triangle_set connector_mesh;

    int sectorCount{1};
    switch (CutConnectorShape(connector_attributes.shape)) {
    case CutConnectorShape::Triangle:
        sectorCount = 3;
        break;
    case CutConnectorShape::Square:
        sectorCount = 4;
        break;
    case CutConnectorShape::Circle:
        sectorCount = 360;
        break;
    case CutConnectorShape::Hexagon:
        sectorCount = 6;
        break;
    default:
        break;
    }

    if (connector_attributes.type == CutConnectorType::Snap)
        connector_mesh = Biz::Algorithms::TriangleMesh::its_make_snap(
            1.0,
            1.0,
            snap_space_proportion,
            snap_bulge_proportion
        );
    else if (connector_attributes.style == CutConnectorStyle::Prism)
        connector_mesh =
            Biz::Algorithms::TriangleMesh::its_make_cylinder(1.0, 1.0, (2 * PI / sectorCount));
    else if (connector_attributes.type == CutConnectorType::Plug)
        connector_mesh =
            Biz::Algorithms::TriangleMesh::its_make_frustum(1.0, 1.0, (2 * PI / sectorCount));
    else
        connector_mesh =
            Biz::Algorithms::TriangleMesh::its_make_frustum_dowel(1.0, 1.0, sectorCount);

    return connector_mesh;
}

void CutGizmo::apply_cut_connectors(ModelObject* mo, const std::string& connector_name)
{
    if (mo->cut_connectors.empty())
        return;

    using namespace Biz::Algorithms::Geometry;

    size_t connector_id = mo->cut_id.connectors_cnt();
    for (const CutConnector& connector : mo->cut_connectors) {
        Domain::TriangleMesh mesh = Domain::TriangleMesh(
            get_connector_mesh(connector.attribs, m_snap_space_proportion, m_snap_bulge_proportion)
        );
        ModelVolume* new_volume = Biz::Algorithms::ModelObject::add_volume(
            mo,
            std::move(mesh),
            ModelVolumeType::NEGATIVE_VOLUME
        );

        // Transform the new modifier to be aligned inside the instance
        new_volume->set_transformation(
            translation_transform(connector.pos)
            * connector.rotation_m
            * rotation_transform(-connector.z_angle * Vec3d::UnitZ())
            * scale_transform(
                Vec3f(connector.radius, connector.radius, connector.height).cast<double>()
            )
        );

        new_volume->cut_info =
            {connector.attribs.type, connector.radius_tolerance, connector.height_tolerance};
        new_volume->name = connector_name + "-" + std::to_string(++connector_id);
    }
    mo->cut_id.increase_connectors_cnt(mo->cut_connectors.size());

    // delete all connectors
    mo->cut_connectors.clear();
}

static void update_object_cut_id(
    CutId& cut_id,
    Biz::ModelObjectCutAttributes attributes,
    const int dowels_count
)
{
    // we don't save cut information, if result will not contains all parts of initial object
    if (!attributes.keep_upper || !attributes.keep_lower || attributes.invalidate_cut_info)
        return;

    if (!cut_id.valid())
        cut_id.init();
    // increase check sum, if it's needed
    {
        int cut_obj_cnt = -1;
        if (attributes.keep_upper)
            cut_obj_cnt++;
        if (attributes.keep_lower)
            cut_obj_cnt++;
        if (attributes.create_dowels)
            cut_obj_cnt += dowels_count;
        if (cut_obj_cnt > 0)
            cut_id.increase_check_sum(size_t(cut_obj_cnt));
    }
}

static void check_objects_after_cut(const ModelObjectPtrs& objects)
{
    std::vector<std::string> err_objects_names;
    std::vector<int> err_objects_idxs;
    int obj_idx{0};
    for (const ModelObject* object : objects) {
        std::vector<std::string> connectors_names;
        connectors_names.reserve(object->volumes.size());
        for (const ModelVolume* vol : object->volumes)
            if (vol->cut_info.is_connector)
                connectors_names.push_back(vol->name);
        const size_t connectors_count = connectors_names.size();

        // remove duplicates
        std::sort(connectors_names.begin(), connectors_names.end());
        connectors_names.erase(
            std::unique(connectors_names.begin(), connectors_names.end()),
            connectors_names.end()
        );

        if (connectors_count != connectors_names.size())
            err_objects_names.push_back(object->name);

        // check manifold/repairs
        // auto stats = ModelProcessing::get_object_mesh_stats(object);
        // if (!stats.manifold() || stats.repaired())
        // err_objects_idxs.push_back(obj_idx);
        obj_idx++;
    }
    /*
        if (!err_objects_names.empty()) {
            wxString names = from_u8(err_objects_names[0]);
            for (size_t i = 1; i < err_objects_names.size(); i++)
                names += ", " + from_u8(err_objects_names[i]);
            WarningDialog(plater, format_wxstr("Objects(%1%) have duplicated connectors. "
                "Some connectors may be missing in slicing result.\n"
                "Please report to PrusaSlicer team in which scenario this issue happened.\n"
                "Thank you.", names)).ShowModal();
        }

        if (is_windows10() && !err_objects_idxs.empty()) {
            auto dlg = WarningDialog(plater, _L("Open edges or errors were detected after the cut.\n"
                "Do you want to fix them by Windows repair algorithm?"),
                _L("Errors detected after cut operation"), wxYES_NO);
            if (dlg.ShowModal() == wxID_YES) {
                //          model_name
                std::vector<std::string>                           succes_models;
                //                    model_name   failing reason
                std::vector<std::pair<std::string, std::string>>   failed_models;

                std::vector<std::string> model_names;

                for (int obj_idx : err_objects_idxs)
                    model_names.push_back(objects[obj_idx]->name);

                auto fix_and_update_progress = [model_names, &objects](const int obj_idx, int model_idx,
                    wxProgressDialog& progress_dlg,
                    std::vector<std::string>& succes_models,
                    std::vector<std::pair<std::string, std::string>>& failed_models) -> bool
                    {
                        const std::string& model_name = model_names[model_idx];
                        wxString msg;
                        if (model_names.size() == 1)
                            msg = GUI::format(_L("Repairing object %1%"), model_name) + "\n";
                        else {
                            // TRN: This is followed by a list of object which are to be repaired.
                            msg = _L("Repairing objects:") + "\n";
                            for (int i = 0; i < int(model_names.size()); ++i)
                                msg += (i == model_idx ? " > " : "   ") + from_u8(model_names[i]) + "\n";
                            msg += "\n";
                        }

                        std::string res;
                        if (!fix_model_by_win10_sdk_gui(*objects[obj_idx], -1, progress_dlg, msg, res))
                            return false;

                        if (res.empty())
                            succes_models.push_back(model_name);
                        else
                            failed_models.push_back({ model_name, res });
                        return true;
                    };

                // Open a progress dialog.
                // TRN: This shows in a progress dialog while the operation is in progress.
                wxProgressDialog progress_dlg(_L("Fixing by Windows repair algorithm"), "", 100, find_toplevel_parent(plater),
                    wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT);
                int model_idx{ 0 };
                for (int obj_idx : err_objects_idxs) {
                    if (!fix_and_update_progress(obj_idx, model_idx, progress_dlg, succes_models, failed_models))
                        break;
                    model_idx++;
                }

                // Close the progress dialog
                progress_dlg.Update(100, "");

                // Show info dialog
                wxString msg = MenuFactory::get_repaire_result_message(succes_models, failed_models);
                // TRN: Title of a dialog informing the user about the result of the model repair operation.
                InfoDialog(plater, _L("Repair operation finished"), msg).ShowModal();
            }
        }
        */
}

void synchronize_model_after_cut(Model& model, const CutId& cut_id)
{
    for (ModelObject* obj : model.objects)
        if (obj->is_cut() && obj->cut_id.has_same_id(cut_id) && !obj->cut_id.is_equal(cut_id))
            obj->cut_id = cut_id;
}

void CutGizmo::perform_cut()
{
    if (!can_perform_cut() || !m_selected_object)
        return;

    // deactivate CutGizmo and than perform a cut
    if (m_callbacks.force_deactivation) {
    // force gizmo deactivation and reset data
        m_callbacks.force_deactivation();
    }

    // perform cut
    {
        // Plater::TakeSnapshot snapshot(wxGetApp().plater(), _L("Cut by Plane"));

        // This shall delete the part selection class and deallocate the memory.
        ScopeGuard part_selection_killer([this]() { m_part_selection = CutPartSelection(); });

        const bool cut_with_groove = !is_planar_mode();

        Domain::ModelObject* cut_mo;
        if (m_cut_by_contour) {
            m_part_selection.model_object()->cut_connectors = m_selected_object->cut_connectors;
            cut_mo                                          = m_part_selection.model_object();
        }
        cut_mo = m_selected_object;

        int dowels_count          = 0;
        const bool has_connectors = !m_selected_object->cut_connectors.empty();
        // update connectors pos as offset of its center before cut performing
        // apply_connectors_in_model(cut_mo , dowels_count);

        // ys_FIXME:: set wxBusyCursor ;

        Biz::ModelObjectCutAttributes attributes = {
            .keep_upper          = has_connectors ? true : keep_upper(),
            .keep_lower          = has_connectors ? true : keep_lower(),
            .keep_as_parts       = has_connectors ? false : keep_as_parts(),
            .flip_upper          = flip_upper(),
            .flip_lower          = flip_lower(),
            .place_on_cut_upper  = place_on_cut_upper(),
            .place_on_cut_lower  = place_on_cut_lower(),
            .create_dowels       = dowels_count > 0,
            .invalidate_cut_info = !has_connectors && !cut_with_groove && !cut_mo->cut_id.valid()
        };

        // update cut_id for the cut object in respect to the attributes
        update_object_cut_id(cut_mo->cut_id, attributes, dowels_count);

        Biz::Cut cut(cut_mo, m_instance_idx, get_cut_matrix(), attributes);
        const ModelObjectPtrs& new_objects = m_cut_by_contour ?
            cut.perform_by_contour(m_part_selection.get_cut_parts(), dowels_count) :
            cut_with_groove ? cut.perform_with_groove(m_groove, m_rotation_m) :
                              cut.perform_with_plane();

        check_objects_after_cut(new_objects);
        m_project_interactor->scene_interactor().delete_object(m_selected_object);

        // save cut_id to post update synchronization
        const CutId cut_id = cut_mo->cut_id;

        // update cut results in the model

        m_project_interactor->scene_interactor().add_new_objects(new_objects);

        // may be better solution?
        synchronize_model_after_cut(m_project_interactor->selected_project().model(), cut_id);
    }
}

void CutGizmo::reset_cut_by_contours()
{
    m_part_selection = CutPartSelection();

    if (!is_planar_mode()) {
        if (/*m_dragging || */ m_groove_editing || !has_valid_groove())
            return;
        process_contours();
    }
    // else
    // toggle_model_objects_visibility();
}

void CutGizmo::process_contours()
{
    if (!m_selected_instance)
        return;

    reset_cut_part_meshes();

    //! wxBusyCursor wait;

    if (is_planar_mode()) {
        reset_cut_by_contours();
        m_part_selection = CutPartSelection(
            m_selected_object,
            get_cut_matrix(),
            m_instance_idx,
            m_plane_center,
            m_cut_normal
        );
    } else {
        if (has_valid_groove()) {
            Biz::Cut cut(m_selected_object, m_instance_idx, get_cut_matrix());
            const ModelObjectPtrs& new_objects =
                cut.perform_with_groove(m_groove, m_rotation_m, true);
            if (!new_objects.empty())
                m_part_selection = CutPartSelection(new_objects.front(), m_instance_idx);
        }
    }
    Scene::Scene& scene = m_scene_presenter.scene();
    size_t part_id{ 0 };
    for (const auto& part : m_part_selection.parts()) {
        Scene::NodeBuilder builder(scene);
        build_cut_part_mesh(
            part.selected ? CutMeshType::UpperPart : CutMeshType::LowerPart,
            part_id++,
            part.mesh,
            part.trafo,
            builder
        );
        scene.add_child(builder.build().release(), m_main_node);
    }
}

bool CutGizmo::keep_upper() const
{
    return m_dialog->keep_upper;
}

bool CutGizmo::keep_lower() const
{
    return m_dialog->keep_lower;
}

bool CutGizmo::place_on_cut_upper() const
{
    return m_dialog->place_on_cut_upper;
}

bool CutGizmo::place_on_cut_lower() const
{
    return m_dialog->place_on_cut_lower;
}

bool CutGizmo::flip_upper() const
{
    return m_dialog->flip_upper;
}

bool CutGizmo::flip_lower() const
{
    return m_dialog->flip_lower;
}

bool CutGizmo::is_planar_mode() const
{
    return m_dialog->is_planar_cut_mode;
}

bool CutGizmo::keep_as_parts() const
{
    return m_dialog->keep_as_parts;
}

} // namespace Slic3r::App::Plater
