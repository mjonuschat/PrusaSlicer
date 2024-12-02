#include "Slic3r/App/Plater/GizmoManager.hpp"

#ifndef DEBUG_GIZMO_MANAGER
#define DEBUG_GIZMO_MANAGER 1
#endif

#if DEBUG_GIZMO_MANAGER
#include "Slic3r/TypeInfo.hpp"
#include "Slic3r/Log.hpp"
#endif


namespace Slic3r::App::Plater {

#if DEBUG_GIZMO_MANAGER
const char* ACTIVATION_STATE_NAMES[] = {
    "Inactive",
    "Probing",
    "Active",
    "Done"
};

#endif

void GizmoManager::on_scene_mouse_event(const Platform::MouseEvent& e, const Slic3r::App::Render::ScreenInfo& screen_info)
{
    if (!m_in_cycle) {
        if (e.is_imgui_captured())
            return;
        prepare_cycle();
    }

    Scene::Scene& scene = m_scene_provider.scene();

    Scene::NodePickResults pick_results;
    Scene::Ray pick_ray;
    scene.pick_at(
        screen_info.mouse_to_screen(e.x()),
        screen_info.mouse_to_screen(e.y()),
        pick_results, &pick_ray

    );

    GizmoEventContext ctx{e, pick_ray, pick_results, screen_info};
    const bool single_active = m_in_cycle_gizmos.size() == 1;
#if DEBUG_GIZMO_MANAGER
    SPDLOG_INFO("process event {} ---in-cycle: {}", int(e.type()), m_in_cycle);
#endif

    auto it = m_in_cycle_gizmos.begin();
    while (it != m_in_cycle_gizmos.end()) {
        auto g = *it;

        auto ret = g->on_mouse(ctx, single_active);
#if DEBUG_GIZMO_MANAGER
        SPDLOG_INFO("- {} with result {}", type_name(*g), ACTIVATION_STATE_NAMES[int(ret)]);
#endif

        if (ret == GizmoActivationState::Inactive) {
            it = m_in_cycle_gizmos.erase(it);
            continue;
        } else if (ret == GizmoActivationState::Done) {
            m_in_cycle_gizmos.clear();
            break;
        } else if (ret == GizmoActivationState::Active) {
            m_in_cycle_gizmos.clear();
            m_in_cycle_gizmos.push_back(g);
            break;
        }
        ++it;
    }

    if (m_in_cycle && m_in_cycle_gizmos.empty())
        m_in_cycle = false;
}

void GizmoManager::prepare_cycle()
{
    m_in_cycle = true;
    m_in_cycle_gizmos.reserve(m_base_gizmos.size() + (m_active_tool != nullptr ? 1 : 0));
    for (const auto& g : m_base_gizmos) {
        g->on_cycle_prepare();
        m_in_cycle_gizmos.push_back(g.get());
    }
    if (m_active_tool)
        m_in_cycle_gizmos.push_back(m_active_tool);
#if DEBUG_GIZMO_MANAGER
    SPDLOG_INFO("New cycle, active gizmos:");
    for (const auto& g : m_in_cycle_gizmos)
        SPDLOG_INFO("- {}", type_name(*g));
#endif
}

void GizmoManager::render_scene(Render::CommandBuffer& cmd_buffer)
{
    for (auto* g : m_in_cycle_gizmos)
        g->render_scene(cmd_buffer);
}

void GizmoManager::render_imgui()
{
    for (auto* g : m_in_cycle_gizmos)
        g->render_imgui();
}

}
