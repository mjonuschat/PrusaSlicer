#include "Slic3r/App/Plater/RotationGizmo.hpp"
#include "Slic3r/App/Plater/GizmoDataFactory.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Render/Device.hpp"

namespace Slic3r::App::Plater {

namespace {

constexpr double HALF_PI = 0.5 * PI;
constexpr double TWO_PI = 2.0 * PI;
constexpr double CIRCLE_RADIUS = 30.0;
constexpr double CIRCLE_DIAMETER = 2.0 * CIRCLE_RADIUS;
static const Vec3d HANDLE_CUBE_SIZE = { 10.0, 10.0, 10.0 };
static const Vec3d HANDLE_CONE_SIZE = { 10.0, 10.0, 15.0 };
static const double HANDLE_STEM_LENGTH = CIRCLE_RADIUS * CIRCLE_FINE_GRADE_PRIMARY_OUT_RADIUS + 0.5 * HANDLE_CUBE_SIZE[X];
constexpr double HANDLE_GAP_LENGTH = 1.0;
static const Vec3d HANDLE_CUBE_OFFSET = { HANDLE_STEM_LENGTH, 0.0, 0.0 };
static const Vec3d HANDLE_CONE_CCW_OFFSET = { HANDLE_STEM_LENGTH, 0.5 * HANDLE_CUBE_SIZE[Y] + HANDLE_GAP_LENGTH, 0.0 };
static const Vec3d HANDLE_CONE_CW_OFFSET = { HANDLE_STEM_LENGTH, -(0.5 * HANDLE_CUBE_SIZE[Y] + HANDLE_GAP_LENGTH), 0.0 };

} // namespace

struct RotationGizmoNodeTag : public GizmoNodeTag
{
    uint8_t level{ 0 };
    bool is_handle{ false };

    explicit RotationGizmoNodeTag(
        AxisType primary_axis,
        AxisType secondary_axis = AxisType::None,
        uint8_t level = 0,
        bool is_handle = false
    )
      : GizmoNodeTag(primary_axis, secondary_axis)
      , level(level)
      , is_handle(is_handle)
    {
    }
};

static Transform3d axis_transform(AxisType axis)
{
    Transform3d ret = Transform3d::Identity();
    switch (axis)
    {
    case AxisType::XAxis:
    {
        ret.rotate(Eigen::AngleAxisd(HALF_PI, Vec3d::UnitY()));
        ret.rotate(Eigen::AngleAxisd(-HALF_PI, Vec3d::UnitZ()));
        break;
    }
    case AxisType::YAxis:
    {
        ret.rotate(Eigen::AngleAxisd(-HALF_PI, Vec3d::UnitZ()));
        ret.rotate(Eigen::AngleAxisd(-HALF_PI, Vec3d::UnitY()));
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

static Vec3d mouse_position_in_local_plane(AxisType axis, const Transform3d& orient_matrix, const Vec3d& center,
    const Linef3& mouse_ray)
{
    Transform3d m = axis_transform(axis).inverse();
    m = m * Geometry::Transformation(orient_matrix).get_matrix_no_offset().inverse();

    m.translate(-center);

    const Linef3 local_mouse_ray = transform(mouse_ray, m);
    if (std::abs(local_mouse_ray.vector().dot(Vec3d::UnitZ())) < EPSILON) {
        // if the ray is parallel to the plane containing the circle
        if (std::abs(local_mouse_ray.vector().dot(Vec3d::UnitY())) > 1.0 - EPSILON)
            // if the ray is parallel to handle direction
            return Vec3d::UnitX();
        else {
            const Vec3d world_pos = (local_mouse_ray.a.x() >= 0.0) ? mouse_ray.a - center : mouse_ray.b - center;
            m.translate(center);
            return m * world_pos;
        }
    }
    else
        return local_mouse_ray.intersect_plane(0.0);
}

static Vec3d extract_position(const App::Scene::Transform& xform)
{
    return xform.block<3, 1>(0, 3);
}

RotationGizmo::RotationGizmo(Render::Device& device, GizmoDataFactory& data_factory,
    ScenePresenter& scene_presenter, Biz::Scene::SceneInteractor& scene_interactor
)
    : m_device(device)
    , m_data_factory(data_factory)
    , m_scene_presenter(scene_presenter)
    , m_scene_interactor(scene_interactor)
{
    m_snap = {
          { CIRCLE_RADIUS * CIRCLE_COARSE_GRADE_IN_RADIUS, CIRCLE_RADIUS * CIRCLE_COARSE_GRADE_OUT_RADIUS },
          { CIRCLE_RADIUS, CIRCLE_RADIUS * CIRCLE_FINE_GRADE_PRIMARY_OUT_RADIUS }
    };
}

GizmoActivationState RotationGizmo::on_mouse(GizmoEventContext& ctx, bool only_active)
{
    const auto event_type = ctx.mouse_event().type();
    if (event_type != Platform::MouseEvent::Type::ButtonDown &&
        event_type != Platform::MouseEvent::Type::Move &&
        event_type != Platform::MouseEvent::Type::ButtonUp) {
        on_stop_dragging();
        return GizmoActivationState::Inactive;
    }

    const auto& pick_ray = ctx.pick_ray();

    if (event_type == Platform::MouseEvent::Type::ButtonDown) {
        const Scene::Node* node = ctx.pick_result_node_with_tag_of_type<RotationGizmoNodeTag>();
        if (node == nullptr) {
            on_stop_dragging();
            return GizmoActivationState::Inactive;
        }

        const RotationGizmoNodeTag& tag = *node->tag_of_type<RotationGizmoNodeTag>();
        m_translation_ray.origin = extract_position(m_scene_presenter.selection_root().world_transform());
        m_translation_ray.direction = tag.primary_axis_dir();
    }

    double t;
    if (!m_translation_ray.closest_point_from_ray(pick_ray, t)) {
        on_stop_dragging();
        return GizmoActivationState::Inactive;
    }

    if (event_type == Platform::MouseEvent::Type::ButtonDown) {
        m_dragging = true;
        m_pivot_world = extract_position(m_scene_presenter.selection_root().world_transform());
        const auto& selection = m_scene_interactor.selection();
        if (selection.mode == Biz::Scene::SelectionMode::Instance) {
            m_pivot_local = m_pivot_world;
        } else {
            auto first_element_ref = selection.elements.front();
            auto parent_ref = Domain::ElementRef{first_element_ref.object_id, first_element_ref.instance_id, 0};

            const Scene::Node *parent_node = ASSERT_VAL(
                m_scene_presenter.scene().root().query_first([&](const auto& node) {
                    auto* node_tag = node->template tag_of_type<SceneNodeTag>();
                    return node_tag && node_tag->matches_element(parent_ref);
                })
            );

            auto parent_world = parent_node->world_transform();
            m_pivot_local = extract_position(
                parent_world.inverse() * m_scene_presenter.selection_root().
                world_transform());
        }
        return GizmoActivationState::Active;
    }

    if (!m_dragging)
        return GizmoActivationState::Inactive;

    if (m_curr_axis != AxisType::None) {
        Vec2d pos = to_2d(mouse_position_in_local_plane(m_curr_axis, Transform3d::Identity(), m_pivot_world,
          Linef3(pick_ray.origin, pick_ray.point_at(10.0))));

        Vec2d orig_dir = Vec2d::UnitX();
        Vec2d new_dir = pos.normalized();

        double theta = acos(std::clamp(new_dir.dot(orig_dir), -1.0, 1.0));
        if (cross2(orig_dir, new_dir) < 0.0)
            theta = TWO_PI - theta;

        double len = pos.norm();

        // take in account that the selection root is scaled to keep the gizmo with constant screen size
        const App::Scene::INodeTransformModifier* modifier = m_scene_presenter.selection_root().transform_modifier();
        if (modifier != nullptr) {
            const App::Scene::Camera& camera = m_scene_presenter.scene().camera();
            double scale = camera.cam_projection()
                .constant_screen_space_size_scale(camera, (m_pivot_world - camera.position()).norm()) * ScenePresenter::screen_space_sized_modifier();
            len /= scale;
        }

        // snap to coarse snap region
        if (m_snap.coarse.in <= len && len <= m_snap.coarse.out) {
            double step = TWO_PI / CIRCLE_COARSE_GRADE_STEPS;
            theta = step * std::round(theta / step);
        }
        else {
            // snap to fine snap region
            if (m_snap.fine.in <= len && len <= m_snap.fine.out) {
                double step = TWO_PI / CIRCLE_FINE_GRADE_SECONDARY_STEPS;
                theta = step * std::round(theta / step);
            }
        }

        if (theta == TWO_PI)
            theta = 0.0;

        Transform3d xform = Transform3d::Identity();
        xform.rotate(Eigen::AngleAxisd(theta, Vec3d::UnitZ()));
        m_handles[size_t(m_curr_axis) - 1]->set_local_transform(xform.matrix());

        xform = Transform3d::Identity();
        xform.translate(m_pivot_local);
        xform.rotate(Eigen::AngleAxisd(theta, axis_type_dir(m_curr_axis)));
        xform.translate(-m_pivot_local);
        m_scene_presenter.set_freeze_selection_center(true);
        m_scene_interactor.transform_selection(xform.matrix(), m_xform_memento);
        m_scene_presenter.set_freeze_selection_center(false);
    }

    if (event_type == Platform::MouseEvent::Type::ButtonUp) {
        m_scene_interactor.finalize_transform_selection(m_xform_memento, false);
        on_stop_dragging();
        clear_highlight();
        return GizmoActivationState::Done;
    }

    return GizmoActivationState::Active;
}

void RotationGizmo::on_transient_mouse(GizmoEventContext& ctx)
{
    if (!m_activated || m_dragging)
        return;

    auto* n = ctx.pick_result_node_with_tag_of_type<RotationGizmoNodeTag>();
    if (n == nullptr) {
        clear_highlight();
        m_curr_axis = AxisType::None;
    } else {
        // when hovering over a handle
        // show only the correspondent axis
        // replacing the circle with the graded circle
        auto* p = n->parent();    // handle
        auto* gp = p->parent();   // {} axis
        auto* ggp = gp->parent(); // main
        for (auto& child : ggp->children()){
            child->set_enabled(child.get() == gp);
        }
        for (auto& child : gp->children()) {
            const RotationGizmoNodeTag& tag = *child->tag_of_type<RotationGizmoNodeTag>();
            child->set_enabled(tag.level >= 1);
        }
        m_highlighted = true;
        m_curr_axis = p->tag_of_type<RotationGizmoNodeTag>()->primary_axis;
    }
}

void RotationGizmo::on_cycle_prepare()
{
    m_dragging = false;
}

static void build_rotate_node(AxisType axis, Scene::NodeBuilder& builder, Render::Device& device, GizmoDataFactory& data_factory)
{
    ColorRGBA color = axis_color(axis);

    builder.set_debug_name(axis_string(axis));
    builder.set_tag(RotationGizmoNodeTag{ axis });

    builder.child([&](Scene::NodeBuilder& bldr) {
        Render::Material material = Render::Material{}
            .set_shader(device.context().shader_manager().get_shader("flat"))
            .set_uniform("uniform_color", color);

        bldr
            .set_debug_name("circle")
            .set_tag(RotationGizmoNodeTag{ axis })
            .set_mesh(data_factory.geometry(GizmoDataId::Circle), material, int(PlaterSceneLayer::GizmoHandles))
            .transform([&](Transform3d& xform) {
                xform.scale(CIRCLE_DIAMETER * Vec3d::Ones());
            });
    });

    builder.child([&](Scene::NodeBuilder& bldr) {
        Render::Material material = Render::Material{}
            .set_shader(device.context().shader_manager().get_shader("flat"))
            .set_uniform("uniform_color", ColorRGBA::WHITE());

        bldr
            .set_debug_name("graded circle")
            .set_tag(RotationGizmoNodeTag{ axis, AxisType::None, 2 })
            .set_mesh(data_factory.geometry(GizmoDataId::GradedCircle), material, int(PlaterSceneLayer::GizmoHandles))
            .set_enabled(false)
            .transform([&](Transform3d& xform) {
                xform.scale(CIRCLE_DIAMETER * Vec3d::Ones());
            });
    });

    builder.child([&](Scene::NodeBuilder& bldr) {
        bldr
            .set_debug_name("handle")
            .set_tag(RotationGizmoNodeTag{ axis, AxisType::None, 1, true });

        bldr.child([&](Scene::NodeBuilder& child_bldr) {
            Render::Material material = Render::Material{}
                .set_shader(device.context().shader_manager().get_shader("flat"))
                .set_uniform("uniform_color", color);

            child_bldr
                .set_debug_name("stem")
                .set_tag(RotationGizmoNodeTag{ axis })
                .set_mesh(data_factory.geometry(GizmoDataId::Segment), material, int(PlaterSceneLayer::GizmoHandles))
                .transform([&](Transform3d& xform) {
                    xform.scale(HANDLE_STEM_LENGTH * Vec3d::UnitX());
                });
        });

        bldr.child([&](Scene::NodeBuilder& child_bldr) {
            auto geom = data_factory.geometry(GizmoDataId::Cube);
            auto  mesh = data_factory.triangle_mesh(GizmoDataId::Cube);

            Render::Material material = Render::Material{}
                .set_shader(device.context().shader_manager().get_shader("gouraud_light"))
                .set_uniform("uniform_color", color);

            child_bldr
                .set_debug_name("cube")
                .set_tag(RotationGizmoNodeTag{ axis })
                .set_mesh(geom, material, int(PlaterSceneLayer::GizmoHandles))
                .set_aabb(mesh->aabb_mesh())
                .transform([&](Transform3d& xform) {
                    xform
                        .translate(HANDLE_CUBE_OFFSET)
                        .scale(HANDLE_CUBE_SIZE);
                });
        });

        bldr.child([&](Scene::NodeBuilder& child_bldr) {
            auto geom = data_factory.geometry(GizmoDataId::Cone);
            auto  mesh = data_factory.triangle_mesh(GizmoDataId::Cone);

            Render::Material material = Render::Material{}
                .set_shader(device.context().shader_manager().get_shader("gouraud_light"))
                .set_uniform("uniform_color", color);

            child_bldr
                .set_debug_name("cone ccw")
                .set_tag(RotationGizmoNodeTag{ axis })
                .set_mesh(geom, material, int(PlaterSceneLayer::GizmoHandles))
                .set_aabb(mesh->aabb_mesh())
                .transform([&](Transform3d& xform) {
                    xform
                        .translate(HANDLE_CONE_CCW_OFFSET)
                        .rotate(Eigen::AngleAxisd{ -HALF_PI, Vec3d::UnitX() })
                        .scale(HANDLE_CONE_SIZE);
                });
        });

        bldr.child([&](Scene::NodeBuilder& child_bldr) {
            auto geom = data_factory.geometry(GizmoDataId::Cone);
            auto  mesh = data_factory.triangle_mesh(GizmoDataId::Cone);

            Render::Material material = Render::Material{}
                .set_shader(device.context().shader_manager().get_shader("gouraud_light"))
                .set_uniform("uniform_color", color);

            child_bldr
                .set_debug_name("cone cw")
                .set_tag(RotationGizmoNodeTag{ axis })
                .set_mesh(geom, material, int(PlaterSceneLayer::GizmoHandles))
                .set_aabb(mesh->aabb_mesh())
                .transform([&](Transform3d& xform) {
                    xform
                        .translate(HANDLE_CONE_CW_OFFSET)
                        .rotate(Eigen::AngleAxisd{ HALF_PI, Vec3d::UnitX() })
                        .scale(HANDLE_CONE_SIZE);
                });
        });
    });
}

void RotationGizmo::on_activated()
{
    m_activated = true;

    auto& scene = m_scene_presenter.scene();
    auto& selection_root = m_scene_presenter.selection_root();

    // builds the following hierarchy of elements:
    // [main] - [X axis]
    //        - [Y axis]
    //        - [Z axis]
    // each of the {} axis elements is composed by the elements:
    // [{} axis] - [circle]
    //           - [graded circle]
    //           - [handle] - [stem]
    //                      - [cube]
    //                      - [cone ccw]
    //                      - [cone cw]

    // circle and graded circle are mutually exclusive:
    // graded circle is shown in place of circle when the gizmo is highlighted.

    Scene::NodeBuilder builder{ scene };
    builder.set_debug_name("main");
    builder.set_tag(RotationGizmoNodeTag{ AxisType::None });

    builder.child([&](Scene::NodeBuilder& bldr) {
        build_rotate_node(AxisType::XAxis, bldr, m_device, m_data_factory);
        bldr.transform([&](Transform3d& xform) {
            xform = axis_transform(AxisType::XAxis) * xform;
        });
    });

    builder.child([&](Scene::NodeBuilder& bldr) {
        build_rotate_node(AxisType::YAxis, bldr, m_device, m_data_factory);
        bldr.transform([&](Transform3d& xform) {
            xform = axis_transform(AxisType::YAxis) * xform;
        });
    });

    builder.child([&](Scene::NodeBuilder& bldr) {
        build_rotate_node(AxisType::ZAxis, bldr, m_device, m_data_factory);
    });

    auto main_node = builder.build();
    scene.add_child(main_node.release(), &selection_root);

    m_handles.clear();
    scene.root().query([this](const Scene::Node* n)->bool {
        const RotationGizmoNodeTag* tag = n->tag_of_type<RotationGizmoNodeTag>();
        return (tag != nullptr && tag->is_handle);
    }, m_handles);

    DEBUG_ASSERT(m_handles.size() == 3);
}

void RotationGizmo::on_deactivated()
{
    m_activated = false;

    m_scene_presenter.scene().remove_children([](const Scene::Node*) { return true; },
        &m_scene_presenter.selection_root());
}

void RotationGizmo::clear_highlight()
{
    if (m_highlighted)
        // show all axes
        // hide graded circle
        visit(
            m_scene_presenter.selection_root(), [](Scene::Node& node) {
                const RotationGizmoNodeTag* tag = node.tag_of_type<RotationGizmoNodeTag>();
                if (tag != nullptr)
                    node.set_enabled(tag->level < 2);
            },
            true
        );
    m_highlighted = false;
}

void RotationGizmo::on_stop_dragging()
{
    std::for_each(m_handles.begin(), m_handles.end(), [](Scene::Node* n) {
        n->set_local_transform(Transform3d::Identity().matrix());
    });
    m_dragging = false;
}

} // namespace Slic3r::App::Plater
