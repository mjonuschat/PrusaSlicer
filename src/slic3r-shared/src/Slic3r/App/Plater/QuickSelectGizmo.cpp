#include "Slic3r/App/Plater/QuickSelectGizmo.hpp"

#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"

namespace Slic3r::App::Plater {

static Scene::Frustum create_frustum(const Scene::Camera& camera, const Render::ScreenInfo& screen_info, const Render::Rect& rect)
{
    Scene::Ray ray_lt = camera.ray_at(screen_info.mouse_to_screen(rect.x), screen_info.mouse_to_screen(rect.y));
    Scene::Ray ray_lb = camera.ray_at(screen_info.mouse_to_screen(rect.x), screen_info.mouse_to_screen(rect.y + rect.height));
    Scene::Ray ray_rt = camera.ray_at(screen_info.mouse_to_screen(rect.x + rect.width), screen_info.mouse_to_screen(rect.y));
    Scene::Ray ray_rb = camera.ray_at(screen_info.mouse_to_screen(rect.x + rect.width), screen_info.mouse_to_screen(rect.y + rect.height));

    const Scene::AbstractCameraProjection& cam_proj = camera.cam_projection();
    double near_z = cam_proj.z_near();
    double far_z  = cam_proj.z_far();

    // near left bottom
    Vec3d nlb = ray_lb.point_at(near_z);
    // near right bottom
    Vec3d nrb = ray_rb.point_at(near_z);
    // near right top
    Vec3d nrt = ray_rt.point_at(near_z);
    // near left top
    Vec3d nlt = ray_lt.point_at(near_z);
    // far left bottom
    Vec3d flb = ray_lb.point_at(far_z);
    // far right bottom
    Vec3d frb = ray_rb.point_at(far_z);
    // far right top
    Vec3d frt = ray_rt.point_at(far_z);
    // far left top
    Vec3d flt = ray_lt.point_at(far_z);

    Scene::Frustum ret;
    ret.vertices = { nlb, nrb, nrt, nlt, flb, frb, frt, flt };
    ret.planes = {
        // near
        Scene::Plane::from_three_points(nrb, nlb, nlt).normalized(),
        // far
        Scene::Plane::from_three_points(flb, frb, flt).normalized(),
        // left
        Scene::Plane::from_three_points(nlb, flb, nlt).normalized(),
        // right
        Scene::Plane::from_three_points(frb, nrb, frt).normalized(),
        // bottom
        Scene::Plane::from_three_points(nlb, nrb, flb).normalized(),
        // top
        Scene::Plane::from_three_points(nrt, nlt, frt).normalized()
    };
    return ret;
}

void RectangleSelection::update(const Vec2i& curr_mouse_pos)
{
    Render::Rect rect{
        std::min(m_initial_mouse_pos.x(), curr_mouse_pos.x()),
        std::min(m_initial_mouse_pos.y(), curr_mouse_pos.y()),
        std::abs(curr_mouse_pos.x() - m_initial_mouse_pos.x()),
        std::abs(curr_mouse_pos.y() - m_initial_mouse_pos.y())
    };

    float scr_w = m_screen_info.logical_width();
    float scr_h = m_screen_info.logical_height();

    ASSERT(scr_w > 0.0f && scr_h > 0.0f);

    float left   = 2.0f * (rect.x / scr_w - 0.5f);
    float right  = 2.0f * ((rect.x + rect.width) / scr_w - 0.5f);
    float top    = -2.0f * (rect.y / scr_h - 0.5f);
    float bottom = -2.0f * ((rect.y + rect.height) / scr_h - 0.5f);

    Render::GeometryBuilder<Render::VertexP3> builder;
    auto* shader = m_device.context().shader_manager().get_shader("flat");
    ASSERT(shader != nullptr);
    builder
        .add_vertex({ {left,  bottom, 0.0f} })
        .add_vertex({ {right, bottom, 0.0f} })
        .add_vertex({ {right, top,    0.0f} })
        .add_vertex({ {left,  top,    0.0f} })
        .add_draw_command({ Render::PrimitiveType::LineLoop, 0, 4, Render::Material{}.set_shader(shader) });
    builder.update(m_geometry);

    m_frustum = create_frustum(m_scene_provider.scene().camera(), m_screen_info, rect);
    m_defined = m_initial_mouse_pos != curr_mouse_pos;
}

static Scene::Node::NodeList extract_instance_nodes(const Scene::Node::NodeList& nodes)
{
    Scene::Node::NodeList ret;
    for (size_t i = 0; i < nodes.size(); ++i) {
        ret.emplace_back(nodes[i]->parent());
    }
    std::sort(ret.begin(), ret.end(),
        [](const Scene::Node* n1, const Scene::Node* n2) { return n1->id() < n2->id(); });
    ret.erase(std::unique(ret.begin(), ret.end(),
        [](const Scene::Node* n1, const Scene::Node* n2) { return n1->id() == n2->id(); }), ret.end());
    return ret;
}

bool RectangleSelection::update_selection(SelectionHandler& selection_handler)
{
    if (!(m_active && m_defined))
        return false;

    Scene::Node::NodeList nodes = collect_contained_nodes();

    if (m_type == Type::Remove) {
        if (m_scene_interactor.selection().mode == Biz::Scene::SelectionMode::Instance)
            nodes = extract_instance_nodes(nodes);
    }
    else if (m_type == Type::Replace) {
        if (std::any_of(nodes.begin(), nodes.end(),
            [&](const Scene::Node* n) {
                return n->tag_of_type<SceneNodeTag>()->volume_type == ModelVolumeType::MODEL_PART;
            }))
            nodes = extract_instance_nodes(nodes);
        else if (nodes.size() != 1) {
            nodes.clear();
            selection_handler.clear_selection();
        }
    }

    for (size_t i = 0; i < nodes.size(); ++i) {
        if (m_type == Type::Remove)
            selection_handler.mark_unselected(*nodes[i]);
        else
            selection_handler.mark_selected(*nodes[i], m_type == Type::Replace && i == 0);
    }

    return true;
}

void RectangleSelection::render(Render::CommandBuffer& cmd_buffer)
{
    if (!(m_active && m_defined))
        return;

    ColorRGBA color;
    switch (m_type)
    {
    case Type::Replace:
    case Type::Add:     { color = ColorRGBA(0.3f, 1.0f, 0.3f, 1.0f); break; }
    case Type::Remove:  { color = ColorRGBA(1.0f, 0.3f, 0.3f, 1.0f); break; }
    default:            { color = ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f); break; }
    }

    Render::Material mat;
    Matrix4f vm = Matrix4f::Identity();
    mat
        .set_uniform("view_model_matrix", vm)
        .set_uniform("projection_matrix", vm)
        .set_uniform("uniform_color", color);
    cmd_buffer.bind_and_draw(m_geometry, mat);
}

Scene::Node::NodeList RectangleSelection::collect_contained_nodes()
{
    Scene::Node::NodeList nodes;
    m_scene_provider.scene().root().query([this](const Scene::Node* n)->bool {
        auto* rcc = n->raycast_component();
        return
          // node has Raycast component present
          rcc != nullptr &&
          // node represents volume/instance
          n->has_tag_of_type<SceneNodeTag>() &&
          // node intersects frustum
          rcc->intersects(n->world_transform(), m_frustum);
    }, nodes);

    return nodes;
}

GizmoActivationState QuickSelectGizmo::on_mouse(GizmoEventContext& ctx, bool only_active)
{
    using namespace std::chrono_literals;
    const auto& evt = ctx.mouse_event();
    auto type = evt.type();

    bool shift_down = (evt.key_modifiers() & Platform::KeyModifiers(Platform::KeyModifier::Shift)) != 0;
    bool ctrl_down  = (evt.key_modifiers() & Platform::KeyModifiers(Platform::KeyModifier::Ctrl)) != 0;
    bool alt_down   = (evt.key_modifiers() & Platform::KeyModifiers(Platform::KeyModifier::Alt)) != 0;

    constexpr static Clock::duration max_click_duration = Clock::duration {500ms};

    auto it =
        std::find_if(ctx.pick_results().begin(), ctx.pick_results().end(), [&](const auto& item) {
            const Scene::Node& n = *item.node;
            return n.has_tag_of_type<SceneNodeTag>();
        });

    if (type == Platform::MouseEvent::Type::ButtonDown) {
        if (evt.button() != Platform::MouseButton::Left) {
            m_processing = false;
            return GizmoActivationState::Inactive;
        }

        RectangleSelection::Type rect_sel_type = (shift_down && ctrl_down) ? RectangleSelection::Type::Add :
                                                 shift_down ? RectangleSelection::Type::Replace :
                                                 alt_down ? RectangleSelection::Type::Remove :
                                                 RectangleSelection::Type::Undefined;
        if (rect_sel_type != RectangleSelection::Type::Undefined)
            m_rectangle_selection.activate(rect_sel_type, { evt.x(), evt.y() });

        if (m_rectangle_selection.is_active())
            return GizmoActivationState::Active;
        else {
            m_click_start = Clock::now();
            m_processing = true;
        }

        return only_active ? GizmoActivationState::Active : GizmoActivationState::Probing;
    }

    if (!m_rectangle_selection.is_active()) {
        if (m_processing && Clock::now() - m_click_start >= max_click_duration) {
            m_processing = false;
            SPDLOG_INFO("QuickSelectGizmo activation timed out");
        }
        if (!m_processing)
            return GizmoActivationState::Inactive;
    }

    if (type == Platform::MouseEvent::Type::Move) {
        if (m_rectangle_selection.is_active()) {
            m_rectangle_selection.update({ evt.x(), evt.y() });
            return GizmoActivationState::Active;
        }
        else
            return GizmoActivationState::Probing;
    } else if (type == Platform::MouseEvent::Type::ButtonUp) {
        if (m_rectangle_selection.is_active()) {
            bool res = m_rectangle_selection.update_selection(m_selection_handler);
            m_rectangle_selection.deactivate();
            if (res)
                return GizmoActivationState::Done;
        }

        const bool additive = shift_down;
        const auto& selection = m_scene_interactor.selection();
        if (selection.empty()) {
            if (it == ctx.pick_results().end())
                return GizmoActivationState::Inactive;

            m_selection_handler.mark_selected(*it->node, !additive);
            return GizmoActivationState::Done;
        } else {
            if (it == ctx.pick_results().end()) {
                if (!additive)
                    m_selection_handler.clear_selection();
                return GizmoActivationState::Done;
            }

            const auto& tag = *it->node->tag_of_type<SceneNodeTag>();

            const bool already_selected =
                std::any_of(selection.elements.begin(), selection.elements.end(), [&](const auto& e) {
                    return e.object_id == tag.object_id && e.volume_id == tag.volume_id &&
                        e.instance_id == tag.instance_id;
                });
            if (additive) {
                if (already_selected) {
                    m_selection_handler.mark_unselected(*it->node);
                } else {
                    m_selection_handler.mark_selected(*it->node, false);
                }
            } else {
                m_selection_handler.mark_selected(*it->node);
            }
            return GizmoActivationState::Done;
        }
    }
    else if (evt.type() == Platform::MouseEvent::Type::Leave) {
        if (m_rectangle_selection.is_active())
            return GizmoActivationState::Active;
    }
    return GizmoActivationState::Inactive;
}

void QuickSelectGizmo::render_scene(Render::CommandBuffer& cmd_buffer)
{
    m_rectangle_selection.render(cmd_buffer);
}

} // namespace Slic3r::App::Plater
