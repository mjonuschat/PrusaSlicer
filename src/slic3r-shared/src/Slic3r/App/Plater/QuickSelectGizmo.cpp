#include "Slic3r/App/Plater/QuickSelectGizmo.hpp"

#include "Slic3r/App/Plater/PlaterGizmosHelper.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/ScopedDebugGroup.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Scene/SceneNodeTag.hpp"

#include "Slic3r/Domain/Color.hpp"

using Slic3r::Domain::SquareMatrix4f;
using Slic3r::Domain::ColorRGBA;
using Slic3r::App::Scene::SceneNodeTag;

namespace Slic3r::App::Plater {

RectangleSelection::RectangleSelection(const Render::ScreenInfo& screen_info, Render::Device& device, Scene::ISceneProvider& scene_provider,
    Biz::Scene::SceneInteractor& scene_interactor)
    : m_screen_info(screen_info)
    , m_device(device)
    , m_scene_provider(scene_provider)
    , m_scene_interactor(scene_interactor)
    , m_geometry(device)
{
    m_material = Render::Material{}
        .set_shader(m_device.context().shader_manager().shader("flat"));

    Render::GeometryBuilder<Render::VertexP3> builder;
    builder
        .add_vertex({ {0.0f, 0.0f, 0.0f} })
        .add_vertex({ {1.0f, 0.0f, 0.0f} })
        .add_vertex({ {1.0f, 1.0f, 0.0f} })
        .add_vertex({ {0.0f, 1.0f, 0.0f} })
        .add_draw_command({ Render::PrimitiveType::LineLoop, 0, 4, m_material });
    builder.update(m_geometry);
}

void RectangleSelection::activate(Type type, const MousePosition& initial_mouse_pos)
{
    m_active = true;
    m_already_processed = false;
    m_type = type;
    m_initial_mouse_pos = initial_mouse_pos;

    ColorRGBA color;
    switch (m_type)
    {
    case Type::Replace:
    case Type::Add:    { color = ColorRGBA(0.3f, 1.0f, 0.3f, 1.0f); break; }
    case Type::Remove: { color = ColorRGBA(1.0f, 0.3f, 0.3f, 1.0f); break; }
    default:           { color = ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f); break; }
    }

    m_material.set_uniform("uniform_color", color);
}

void RectangleSelection::update(const MousePosition& curr_mouse_pos)
{
    Render::Rect rect{
        std::min(m_initial_mouse_pos[0], curr_mouse_pos[0]),
        std::min(m_initial_mouse_pos[1], curr_mouse_pos[1]),
        std::abs(curr_mouse_pos[0] - m_initial_mouse_pos[0]),
        std::abs(curr_mouse_pos[1] - m_initial_mouse_pos[1])
    };

    float scr_w = m_screen_info.logical_width();
    float scr_h = m_screen_info.logical_height();

    DEBUG_ASSERT(scr_w > 0.0f && scr_h > 0.0f);

    float left   = 2.0f * (rect.x / scr_w - 0.5f);
    float right  = 2.0f * ((rect.x + rect.width) / scr_w - 0.5f);
    float top    = -2.0f * (rect.y / scr_h - 0.5f);
    float bottom = -2.0f * ((rect.y + rect.height) / scr_h - 0.5f);

    SquareMatrix4f vm = SquareMatrix4f::Identity();
    vm(0, 0) = right - left;
    vm(1, 1) = top - bottom;
    vm(0, 3) = left;
    vm(1, 3) = bottom;
    m_material.set_uniform("projection_view_model_matrix", vm);

    m_frustum = Scene::Frustum::from(m_scene_provider.scene().camera(), m_screen_info, rect);
    m_defined = m_initial_mouse_pos != curr_mouse_pos;
    m_contained_nodes = collect_contained_nodes();
}

static Scene::Node::NodeList extract_instance_nodes(const Scene::Node::NodeList& nodes)
{
    Scene::Node::NodeList ret;
    for (auto n : nodes) {
        ret.emplace_back(n->parent());
    }
    std::sort(ret.begin(), ret.end(),
        [](const Scene::Node* a, const Scene::Node* b) { return a->id() < b->id(); });
    ret.erase(std::unique(ret.begin(), ret.end(),
        [](const Scene::Node* a, const Scene::Node* b) { return a->id() == b->id(); }), ret.end());
    return ret;
}

static Scene::Node::NodeList all_nodes_from_same_instance(const Scene::Node& node, Scene::Scene& scene)
{
    Scene::Node::NodeList ret;
    const auto tag = node.tag_of_type<SceneNodeTag>();
    DEBUG_ASSERT(tag != nullptr);
    scene.root().query([&](const Scene::Node* n) {
        const auto t = n->tag_of_type<SceneNodeTag>();
        return (t == nullptr) ? false : tag->object_id == t->object_id && tag->instance_id == t->instance_id && t->volume_id != 0;
    }, ret);
    return ret;
}

static bool are_all_nodes_from_same_instance(const Scene::Node::NodeList& nodes)
{
    if (nodes.empty())
        return false;

    size_t inst_id = nodes.front()->parent()->id();
    return std::all_of(nodes.begin(), nodes.end(), [&](const Scene::Node* n) {
        return n->parent()->id() == inst_id;
    });
}

static bool contains_any_part(const Scene::Node::NodeList& nodes)
{
    return nodes.empty() ? false :
           std::any_of(nodes.begin(), nodes.end(),
               [&](const Scene::Node* n) { 
                   return n->tag_of_type<SceneNodeTag>()->volume_type == Domain::ModelVolumeType::MODEL_PART;
               }
           );
}

bool RectangleSelection::update_selection(SelectionHandler& selection_handler)
{
    DEBUG_ASSERT(m_active);
    if (!m_defined)
        return false;

    Scene::Node::NodeList nodes = m_contained_nodes;

    if (m_type == Type::Remove) {
        if (m_scene_interactor.object_selection().mode == Biz::Scene::SelectionMode::Instance)
            nodes = extract_instance_nodes(nodes);
    }
    else if (m_type == Type::Replace) {
        if (std::any_of(nodes.begin(), nodes.end(),
            [&](const Scene::Node* n) {
                return n->tag_of_type<SceneNodeTag>()->volume_type == Domain::ModelVolumeType::MODEL_PART;
            }))
            nodes = extract_instance_nodes(nodes);
        else if (nodes.size() != 1) {
            if (m_scene_interactor.object_selection().mode == Biz::Scene::SelectionMode::Volume &&
                !are_all_nodes_from_same_instance(nodes)) {
                nodes.clear();
                selection_handler.clear_selection();
            }
        }
    }

    for (size_t i = 0; i < nodes.size(); ++i) {
        if (m_type == Type::Remove)
            selection_handler.mark_unselected(*nodes[i]);
        else
            selection_handler.mark_selected(*nodes[i], m_type == Type::Replace && i == 0);
    }

    m_already_processed = true;
    return true;
}

void RectangleSelection::render(Render::CommandBuffer& cmd_buffer)
{
    if (!m_active || !m_defined || m_already_processed)
        return;
    cmd_buffer.bind_and_draw(m_geometry, m_material);
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
          rcc->intersects(n->world_transform().matrix(), m_frustum);
    }, nodes);

    return nodes;
}

static void promote_hover_data_to_full_instance(HoverData& hover_data, const Scene::Node& node, Scene::Scene& scene)
{
    hover_data.nodes = all_nodes_from_same_instance(node, scene);
}

static void promote_hover_data_to_full_instances(HoverData& hover_data, const Scene::Node::NodeList& instances, Scene::Scene& scene)
{
    Scene::Node::NodeList nodes;
    scene.root().query([&](const Scene::Node* n) {
        const auto tag = n->tag_of_type<SceneNodeTag>();
        return (tag == nullptr || tag->volume_id == 0) ? false : 
            std::find_if(instances.begin(), instances.end(), [&](const Scene::Node* i) {
                const auto t = i->tag_of_type<SceneNodeTag>();
                return (t->object_id == tag->object_id && t->instance_id == tag->instance_id);
            }) != instances.end();
    }, nodes);
    hover_data.nodes = nodes;
}

static void refine_hover_data(HoverData& hover_data, bool modifier_pressed, Biz::Scene::ObjectSelection selection, Scene::Scene& scene)
{
    DEBUG_ASSERT(hover_data.nodes.size() == 1);
    if (hover_data.nodes.size() != 1)
        return;

    const auto first_node = hover_data.nodes.front();
    const auto tag = first_node->tag_of_type<SceneNodeTag>();
    if (selection.mode == Biz::Scene::SelectionMode::Volume && !selection.empty()) {
        Domain::ElementRef ref{ tag->object_id, tag->instance_id, 0 };
        if (std::any_of(selection.elements.begin(), selection.elements.end(),
            [&](const Domain::ElementRef& r) { return r.is_part_of(ref); })) {
            if (tag->volume_type == Domain::ModelVolumeType::MODEL_PART)
                promote_hover_data_to_full_instance(hover_data, *first_node, scene);
        }
        else
            hover_data.nodes = Scene::Node::NodeList();
    }
    else if (selection.mode == Biz::Scene::SelectionMode::Instance && !selection.empty()) {
        if (tag->volume_type == Domain::ModelVolumeType::MODEL_PART || modifier_pressed)
            promote_hover_data_to_full_instance(hover_data, *first_node, scene);
    }
    else if (tag->volume_type == Domain::ModelVolumeType::MODEL_PART ||
        (selection.mode == Biz::Scene::SelectionMode::Instance && hover_data.type == HoverType::Unselect))
        promote_hover_data_to_full_instance(hover_data, *first_node, scene);
}

static void refine_rectangle_hover_data(HoverData& hover_data, Biz::Scene::ObjectSelection selection, RectangleSelection::Type type,
    Scene::Scene& scene)
{
    if (type == RectangleSelection::Type::Remove && !selection.empty()) {
        // filters out unselected nodes
        Scene::Node::NodeList nodes;
        nodes.reserve(hover_data.nodes.size());
        for (auto n : hover_data.nodes) {
            const auto tag = n->tag_of_type<SceneNodeTag>();
            if (selection.is_selected(Domain::ElementRef(tag->object_id, tag->instance_id, tag->volume_id)))
                nodes.emplace_back(n);
        }
        hover_data.nodes = nodes;
    }

    if (hover_data.nodes.empty())
        return;

    bool promote_to_full_instance = false;

    if (selection.mode == Biz::Scene::SelectionMode::Volume && !selection.empty()) {
        size_t inst_id = selection.elements.front().instance_id;
        if (type == RectangleSelection::Type::Remove) {
            Scene::Node::NodeList nodes;
            nodes.reserve(hover_data.nodes.size());
            for (auto n : hover_data.nodes) {
                if (n->tag_of_type<SceneNodeTag>()->instance_id == inst_id)
                    nodes.emplace_back(n);
            }
            hover_data.nodes = nodes;
            return;
        }
        else {
            if (std::any_of(hover_data.nodes.begin(), hover_data.nodes.end(),
                [&](const Scene::Node* n) {
                    return n->tag_of_type<SceneNodeTag>()->instance_id != inst_id;
               })
             ) {
                hover_data.nodes = Scene::Node::NodeList();
                return;
            }
    
            if (contains_any_part(hover_data.nodes))
                promote_to_full_instance = true;
            else
                return;
        }
    }
    else
        promote_to_full_instance = type == RectangleSelection::Type::Add ||
                                   type == RectangleSelection::Type::Remove ||
                                   contains_any_part(hover_data.nodes);

    if (promote_to_full_instance)
        promote_hover_data_to_full_instances(hover_data, extract_instance_nodes(hover_data.nodes), scene);
    else if (!are_all_nodes_from_same_instance(hover_data.nodes))
        hover_data.nodes = Scene::Node::NodeList();
}

Scene::GizmoActivationState QuickSelectGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    using namespace std::chrono_literals;
    const auto& evt = ctx.mouse_event();
    auto type = evt.type();

    const auto& selection = m_scene_interactor.object_selection();

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
            return Scene::GizmoActivationState::Inactive;
        }

        RectangleSelection::Type rect_sel_type = (shift_down && ctrl_down) ? RectangleSelection::Type::Add :
                                                 shift_down ? RectangleSelection::Type::Replace :
                                                 alt_down ? RectangleSelection::Type::Remove :
                                                 RectangleSelection::Type::Undefined;
        if (rect_sel_type == RectangleSelection::Type::Add && selection.empty())
            rect_sel_type = RectangleSelection::Type::Replace;

        if (rect_sel_type != RectangleSelection::Type::Undefined)
            m_rectangle_selection.activate(rect_sel_type, { evt.x(), evt.y() });

        if (m_rectangle_selection.is_active() && !m_rectangle_selection.is_already_processed())
            return Scene::GizmoActivationState::Active;
        else {
            m_click_start = Clock::now();
            m_processing = true;
        }

        return (only_active && m_hover_data.nodes.empty()) ?
            Scene::GizmoActivationState::Active : Scene::GizmoActivationState::Probing;
    }

    if (!m_rectangle_selection.is_active()) {
        if (m_processing && Clock::now() - m_click_start >= max_click_duration) {
            m_processing = false;
            SPDLOG_INFO("QuickSelectGizmo activation timed out");
        }
    }

    // replace the following line with
    // bool modifier_pressed = ctrl_down;
    // if you want to use [Ctrl] instead of [Shift]
    bool modifier_pressed = shift_down;
    const SceneNodeTag* tag = nullptr;
    if (it != ctx.pick_results().end())
        tag = it->node->tag_of_type<SceneNodeTag>();

    bool already_selected = (tag == nullptr) ? false : selection.is_selected({ tag->object_id, tag->instance_id, tag->volume_id });

    if (type == Platform::MouseEvent::Type::Move) {
        if (m_rectangle_selection.is_active() && !m_rectangle_selection.is_already_processed()) {
            m_rectangle_selection.update({ evt.x(), evt.y() });
            HoverData hover_data{ shift_down ? HoverType::Select : HoverType::Unselect, m_rectangle_selection.contained_nodes() };
            refine_rectangle_hover_data(hover_data, selection, m_rectangle_selection.type(), m_scene_provider.scene());
            m_rectangle_selection.set_contained_nodes(hover_data.nodes);
            invoke_hover_changed(hover_data);
            return Scene::GizmoActivationState::Active;
        }
        else {
            if (ctx.pick_results_contains_gizmo_nodes()) {
                HoverData hover_data = { HoverType::Select, Scene::Node::NodeList() };
                invoke_hover_changed(hover_data);
                return Scene::GizmoActivationState::Inactive;
            }

            HoverType type = (modifier_pressed && already_selected) ? HoverType::Unselect : HoverType::Select;
            Scene::Node::NodeList nodes;
            if (tag != nullptr)
                nodes = Scene::Node::NodeList{ it->node };

            HoverData hover_data = { type, nodes };
            if (!hover_data.nodes.empty())
                refine_hover_data(hover_data, modifier_pressed, selection, m_scene_provider.scene());
            invoke_hover_changed(hover_data);
            return Scene::GizmoActivationState::Inactive;
        }
    } else if (type == Platform::MouseEvent::Type::ButtonUp) {
        if (m_rectangle_selection.is_active()) {
            if (m_rectangle_selection.is_already_processed()) {
                m_rectangle_selection.deactivate();
                return Scene::GizmoActivationState::Done;
            }

            bool res = m_rectangle_selection.update_selection(m_selection_handler);
            m_rectangle_selection.deactivate();
            if (res)
                return Scene::GizmoActivationState::Done;
        }

        if (selection.empty()) {
            if (it == ctx.pick_results().end())
                return Scene::GizmoActivationState::Inactive;

            m_selection_handler.mark_selected(*it->node);
            return Scene::GizmoActivationState::Done;
        } else {
            if (it == ctx.pick_results().end()) {
                if (!modifier_pressed)
                    m_selection_handler.clear_selection();
                return Scene::GizmoActivationState::Done;
            }

            if (!can_be_added_to_object_selection(*it->node, selection)) {
                m_selection_handler.clear_selection();
                return Scene::GizmoActivationState::Done;
            }

            if (already_selected && selection.mode == Biz::Scene::SelectionMode::Instance && m_hover_data.type == HoverType::Select) {
                if (tag->volume_type != Domain::ModelVolumeType::MODEL_PART) {
                    m_selection_handler.clear_selection();
                    m_selection_handler.mark_selected(*it->node);
                    return Scene::GizmoActivationState::Done;
                }
            }

            if (modifier_pressed) {
                if (already_selected)
                    m_selection_handler.mark_unselected(*it->node);
                else
                    m_selection_handler.mark_selected(*it->node, false);
            }
            else
                m_selection_handler.mark_selected(*it->node);

            return Scene::GizmoActivationState::Done;
        }
    }
    else if (evt.type() == Platform::MouseEvent::Type::Leave) {
        if (m_rectangle_selection.is_active())
            return Scene::GizmoActivationState::Active;
    }
    return Scene::GizmoActivationState::Inactive;
}

void QuickSelectGizmo::render_scene(Render::CommandBuffer& cmd_buffer)
{
    Render::ScopedDebugGroup event_gizmo_manager("Quick Select Gizmo", cmd_buffer);
    m_rectangle_selection.render(cmd_buffer);
}

void QuickSelectGizmo::on_keyboard(Scene::GizmoKeyEventContext& ctx)
{
    const Platform::KeyboardEvent& evt = ctx.keyboard_event();
    if (evt.is_repeat())
        return;

    Platform::KeyCode code = evt.code();
    // replace the following line with
    // bool is_modifier_key = code == Platform::KeyCode::RCtrl || code == Platform::KeyCode::LCtrl;
    // if you want to use [Ctrl] instead of [Shift]
    bool is_modifier_key = code == Platform::KeyCode::RShift || code == Platform::KeyCode::LShift;
    bool is_alt_key = code == Platform::KeyCode::RAlt || code == Platform::KeyCode::LAlt;
    if (!is_modifier_key && !is_alt_key)
        return;

    if (m_rectangle_selection.is_active()) {
        if (evt.type() == Platform::KeyboardEvent::Type::KeyUp)
            m_rectangle_selection.update_selection(m_selection_handler);
    }
    else {
        if (is_modifier_key) {
            HoverData hover_data = m_hover_data;
            bool modifier_pressed = evt.type() == Platform::KeyboardEvent::Type::KeyDown;
            hover_data.type = modifier_pressed ? HoverType::Unselect : HoverType::Select;
            if (hover_data.nodes.size() == 1)
                refine_hover_data(hover_data, modifier_pressed, m_scene_interactor.object_selection(), m_scene_provider.scene());
            if (hover_data.type == HoverType::Unselect && !hover_data.nodes.empty()) {
                // override type to comply with old PrusaSlicer behavior
                const auto tag = hover_data.nodes.front()->tag_of_type<SceneNodeTag>();
                if (!m_scene_interactor.object_selection().is_selected({ tag->object_id, tag->instance_id, tag->volume_id }))
                    hover_data.type = HoverType::Select;
            }
            invoke_hover_changed(hover_data);
        }
    }
}

void QuickSelectGizmo::invoke_hover_changed(const HoverData& hover_data)
{
    if (m_hover_data != hover_data) {
        m_hover_data = hover_data;
        invoke_listeners<IHoverChangedListener>(
            [&](IHoverChangedListener* l) { l->on_hover_changed(m_hover_data); }
        );
    }
}

} // namespace Slic3r::App::Plater
