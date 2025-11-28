#include "Slic3r/App/Plater/TranslationGizmo.hpp"
#include "Slic3r/App/Scene/GeometryDataFactory.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <numbers>

#include "Slic3r/Domain/Color.hpp"


namespace Slic3r::App::Plater {


/*
*/




using Domain::ColorRGBA;
using Domain::SquareMatrix4d;
using Domain::Transform3d;
using Domain::Vec3d;

constexpr double HALF_PI = 0.5 * std::numbers::pi;
static const Vec3d HANDLE_CONE_SIZE = { 10.0, 10.0, 15.0 };
constexpr double HANDLE_STEM_LENGTH = 50.0;
static const Vec3d HANDLE_CONE_OFFSET = { 0.0, 0.0, HANDLE_STEM_LENGTH };

static Transform3d axis_transform(AxisType axis)
{
    Transform3d ret = Transform3d::Identity();
    switch (axis)
    {
    case AxisType::XAxis:
    {
        ret.rotate(Eigen::AngleAxisd(HALF_PI, Vec3d::UnitY()));
        break;
    }
    case AxisType::YAxis:
    {
        ret.rotate(Eigen::AngleAxisd(-HALF_PI, Vec3d::UnitX()));
        break;
    }
    default:
    case AxisType::ZAxis:
    {
        // no rotation applied
        break;
    }
    }
    return ret;
}

static void build_handle_nodes(
    AxisType axis,
    Scene::NodeBuilder& builder,
    Render::Device& device,
    Scene::GeometryDataFactory& data_factory
)
{
    ColorRGBA color = axis_color(axis);

    builder.set_debug_name(axis_string(axis));
    builder.set_tag(TranslationGizmoNodeTag{ axis });

    builder.child([&](Scene::NodeBuilder& bldr) {
        Render::Material material = Render::Material{}
            .set_shader(device.context().shader_manager().shader("flat"))
            .set_uniform("uniform_color", color);

        bldr
            .set_debug_name("stem")
            .set_tag(TranslationGizmoNodeTag{ axis })
            .set_mesh(data_factory.geometry(Scene::GeometryDataId::Segment), material, Scene::RenderLayerId(PlaterSceneLayer::GizmoHandles))
            .set_material_override(material)
            .transform([](Transform3d& xform) {
                xform.rotate(Eigen::AngleAxisd(-HALF_PI, Vec3d::UnitY()));
                xform.scale(HANDLE_STEM_LENGTH * Vec3d::UnitX());
            });
    });

    builder.child([&](Scene::NodeBuilder& bldr) {
        auto geom = data_factory.geometry(Scene::GeometryDataId::Cone);
        auto mesh = data_factory.triangle_mesh(Scene::GeometryDataId::Cone);

        Render::Material material = Render::Material{}
            .set_shader(device.context().shader_manager().shader("gouraud_light"))
            .set_uniform("uniform_color", color);

        bldr
            .set_debug_name("cone")
            .set_tag(TranslationGizmoNodeTag{ axis })
            .set_mesh(geom, material, Scene::RenderLayerId(PlaterSceneLayer::GizmoHandles))
            .set_material_override(material)
            .set_aabb(mesh->aabb_mesh())
            .transform([](Transform3d& xform) {
                xform
                    .translate(HANDLE_CONE_OFFSET)
                    .scale(HANDLE_CONE_SIZE);
            });
    });
}

TranslationGizmo::TranslationGizmo(
    Render::Device& device,
    Scene::GeometryDataFactory& data_factory,
    PlaterScenePresenter& scene_provider,
    Biz::ProjectInteractor& project_interactor
) :
    m_device(device),
    m_data_factory(data_factory),
    m_scene_provider(scene_provider),
    m_scene_interactor(project_interactor.scene_interactor()),
    m_project_interactor(project_interactor)
{
}

void TranslationGizmo::on_cycle_prepare()
{
    m_dragging = false;
}

Scene::GizmoActivationState TranslationGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    const auto event_type = ctx.mouse_event().type();
    if (event_type != Platform::MouseEvent::Type::ButtonDown &&
        event_type != Platform::MouseEvent::Type::Move &&
        event_type != Platform::MouseEvent::Type::ButtonUp) {
        m_dragging = false;
        return Scene::GizmoActivationState::Inactive;
    }

    const auto& pick_ray = ctx.pick_ray();

    if (event_type == Platform::MouseEvent::Type::ButtonDown) {
        const std::optional<Scene::OrientedBoundingBox> obb{m_scene_provider.selection_bounding_box()};
        const Scene::Node* node = ctx.pick_result_node_with_tag_of_type<TranslationGizmoNodeTag>();
        if (node == nullptr || !obb) {
            m_dragging = false;
            return Scene::GizmoActivationState::Inactive;
        }

        const TranslationGizmoNodeTag& tag = *node->tag_of_type<TranslationGizmoNodeTag>();
        Transform3d transform{Transform3d::Identity()};
        transform.translate(obb->center);
        transform.rotate(obb->rotation);
        m_translation_ray
            .origin = transform.translation();
        m_translation_ray.direction = transform.rotation() * tag.primary_axis_dir();
    }

    double t;
    if (!m_translation_ray.closest_point_from_ray(pick_ray, t)) {
        m_dragging = false;
        return Scene::GizmoActivationState::Inactive;
    }

    if (event_type == Platform::MouseEvent::Type::ButtonDown) {
        m_start_t = t;
        m_dragging = true;
        return Scene::GizmoActivationState::Active;
    }

    if (!m_dragging)
        return Scene::GizmoActivationState::Inactive;

    Vec3d delta = m_translation_ray.point_at(t) - m_translation_ray.point_at(m_start_t);

    SquareMatrix4d translation_matrix = SquareMatrix4d::Identity();
    translation_matrix.col(3).head(3) = delta;

    m_scene_interactor
        .transform_selection(translation_matrix, m_xform_memento);

    if (event_type == Platform::MouseEvent::Type::ButtonUp) {
        m_scene_interactor.finalize_transform_selection(m_xform_memento, false);
        m_dragging = false;
        clear_highlight();
        return Scene::GizmoActivationState::Done;
    }

    return Scene::GizmoActivationState::Active;
}

void TranslationGizmo::clear_highlight()
{
    if (m_highlighted) {
        // show all axes
        Scene::Node* handle_nodes{get_handle_nodes()};
        if (handle_nodes != nullptr) {
            visit(
                *handle_nodes, [](Scene::Node& node) { node.set_enabled(true); },
                true
            );
        }
    }
    m_highlighted = false;
}

void TranslationGizmo::on_transient_mouse(Scene::GizmoEventContext& ctx)
{
    if (!m_activated || m_dragging)
        return;
    auto* n = ctx.pick_result_node_with_tag_of_type<TranslationGizmoNodeTag>();
    if (n == nullptr) {
        clear_highlight();
    } else {
        // when hovering over a handle
        // show only the correspondent axis
        auto* p = n->parent();  // {} axis
        auto* gp = p->parent(); // main
        for (auto& child : gp->children()){
            child->set_enabled(child.get() == p);
        }
        m_highlighted = true;
    }
}

void TranslationGizmo::on_activated()
{
    m_activated = true;
    m_window->on_activated();
    Scene::Scene& scene{m_scene_provider.scene()};
    scene.add_child(generate_handle_nodes().release(), &m_scene_provider.selection_root());
}

void TranslationGizmo::on_deactivated()
{
    m_activated = false;
    m_window->on_deactivated();
    m_scene_provider.clear_selection_root_children();
}

std::unique_ptr<Yoga::GizmoWindow> TranslationGizmo::release_ui_window()
{
    auto window{std::make_unique<TranslationDialog>(
        m_scene_provider,
        m_project_interactor
    )};
    m_window = window.get();
    return window;
}

std::unique_ptr<Scene::Node> TranslationGizmo::generate_handle_nodes() const {
    auto& scene = m_scene_provider.scene();

    // builds the following hierarchy of elements:
    // [main] - [X axis]
    //        - [Y axis]
    //        - [Z axis]
    // each of the {} axis elements is composed by the elements:
    // [{} axis] - [stem]
    //           - [cone]

    Scene::NodeBuilder builder{ scene };
    builder.set_debug_name("main");
    builder.set_tag(TranslationGizmoNodeTag{ AxisType::None });

    builder.child([&](Scene::NodeBuilder& bldr) {
        build_handle_nodes(AxisType::XAxis, bldr, m_device, m_data_factory);
        bldr.transform([](Transform3d& xform) {
            xform = axis_transform(AxisType::XAxis) * xform;
        });
    });

    builder.child([&](Scene::NodeBuilder& bldr) {
        build_handle_nodes(AxisType::YAxis, bldr, m_device, m_data_factory);
        bldr.transform([](Transform3d& xform) {
            xform = axis_transform(AxisType::YAxis) * xform;
        });
    });

    builder.child([&](Scene::NodeBuilder& bldr) {
        build_handle_nodes(AxisType::ZAxis, bldr, m_device, m_data_factory);
    });

    return builder.build();
}

Scene::Node* TranslationGizmo::get_handle_nodes() const {
    Scene::Scene& scene{m_scene_provider.scene()};
    Scene::Node::NodeList nodes;
    scene.root().query(
        [&](const Scene::Node* node)
        {
            const auto tag{node->tag_of_type<TranslationGizmoNodeTag>()};
            if (tag == nullptr) {
                return false;
            }
            return tag->primary_axis == AxisType::None;
        },
        nodes
    );
    if (nodes.empty()) {
        return nullptr;
    }
    ASSERT(nodes.size() == 1);
    return nodes.front();
}
}
