#include "Slic3r/App/Plater/GizmoManager.hpp"

#ifndef DEBUG_GIZMO_MANAGER
#define DEBUG_GIZMO_MANAGER 1
#endif

#if DEBUG_GIZMO_MANAGER
#include "Slic3r/TypeInfo.hpp"
#include "Slic3r/Log.hpp"
#include <imgui/imgui.h>
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
    update_gizmo_activation_debug_frame_begin();
#endif

    auto it = m_in_cycle_gizmos.begin();
    while (it != m_in_cycle_gizmos.end()) {
        auto g = *it;

        auto ret = g->on_mouse(ctx, single_active);
#if DEBUG_GIZMO_MANAGER
        SPDLOG_INFO("- {} with result {}", type_name(*g), ACTIVATION_STATE_NAMES[int(ret)]);
        update_gizmo_activation_debug_data(g, ret);
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

    // process transient events
    for (auto& g : m_base_gizmos)
        g->on_transient_mouse(ctx);
    for (auto& g : m_tool_gizmos)
        g->on_transient_mouse(ctx);
}

bool GizmoManager::on_scene_keyboard_event(const Platform::KeyboardEvent& e)
{
    return m_command_registry.process_keyboard_event(e);
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
    // Most gizmos will render on top of scene, so disable depth test here so gizmos shouldn't care
    cmd_buffer.set_depth_test_enabled(false);
    for (auto* g : m_in_cycle_gizmos)
        g->render_scene(cmd_buffer);
    //m_scene_provider.scene().log_nodes();
}

void GizmoManager::render_imgui()
{
    for (auto* g : m_in_cycle_gizmos)
        g->render_imgui();
#if DEBUG_GIZMO_MANAGER
    render_gizmo_activation_debug();
#endif
}

void GizmoManager::activate_tool(ToolType tool, PrinterTechnology pt)
{
    deactivate_current_tool();

    m_active_tool = DEBUG_ASSERT_VAL(find_tool(tool, pt));

    if (m_active_tool != nullptr)
        m_active_tool->on_activated();
}

void GizmoManager::toggle_activate_tool(ToolType tool, PrinterTechnology pt)
{
    IToolGizmo* original_tool = m_active_tool;
    deactivate_current_tool();

    IToolGizmo* next_tool = DEBUG_ASSERT_VAL(find_tool(tool, pt));
    if (next_tool != original_tool) {
        m_active_tool = next_tool;
        m_active_tool->on_activated();
    }
}

void GizmoManager::deactivate_current_tool()
{
    if (m_active_tool == nullptr)
        return;
    m_active_tool->on_deactivated();
    m_active_tool = nullptr;

}

IToolGizmo* GizmoManager::find_tool(ToolType tool, PrinterTechnology pt)
{
    auto it =
        std::find_if(m_tool_gizmos.begin(), m_tool_gizmos.end(), [tool, pt](const IToolGizmoPtr& tg) {
            return tg->type() == tool && tg->supports_printer(pt);
        });
    return it == m_tool_gizmos.end() ? nullptr : it->get();
}


#if DEBUG_GIZMO_MANAGER
void GizmoManager::update_gizmo_activation_debug_data(const IGizmo* g, GizmoActivationState state)
{
    if (auto it = m_activation_debug.find(g); it != m_activation_debug.end()) {
        it->second.back() = state;
    } else {
        m_activation_debug[g] = {state};
    }
}

void GizmoManager::update_gizmo_activation_debug_frame_begin()
{
    for (auto& [g, gad] : m_activation_debug) {
        gad.push_back(GizmoActivationState::Inactive);
        if (gad.size() > NUM_DEBUG_ACTIVATION_LAST_STEPS)
            gad.pop_front();
    }
}

void GizmoManager::render_gizmo_activation_debug()
{
    if (!m_activation_debug_shown)
        return;
    if (ImGui::Begin("Gizmo Activation", &m_activation_debug_shown)) {
        // Begin a table with 2 + maxCellsPerRow columns
        if (ImGui::BeginTable("MapDataTable", 1 + NUM_DEBUG_ACTIVATION_LAST_STEPS, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            // Set fixed width for the first column
            ImGui::TableSetupColumn("Gizmo", ImGuiTableColumnFlags_WidthFixed, 250.0f); // Adjust 150.0f as needed
            for (int i = 0; i < NUM_DEBUG_ACTIVATION_LAST_STEPS; ++i) {
                ImGui::TableSetupColumn("");
            }
            ImGui::TableHeadersRow();

            // Loop through each key-value pair in the map
            for (const auto& [key, values] : m_activation_debug) {
                // Create a new row for the current key
                ImGui::TableNextRow();

                // Render the key in the first column
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(type_name(*key).c_str());

                // Prepare a buffer for rendering values with padding
                int remainingCells = NUM_DEBUG_ACTIVATION_LAST_STEPS - static_cast<int>(values.size());

                // Render blank cells for padding on the left
                for (int cellIndex = 0; cellIndex < remainingCells; ++cellIndex) {
                    ImGui::TableSetColumnIndex(1 + cellIndex);
                    ImGui::TextUnformatted("");
                }

                // Render up to maxCellsPerRow cells for the values
                auto it = values.begin();
                for (int cellIndex = remainingCells; cellIndex < NUM_DEBUG_ACTIVATION_LAST_STEPS; ++cellIndex) {
                    ImGui::TableSetColumnIndex(1 + cellIndex);

                    if (it != values.end()) {
                        // Determine the cell color based on the value
                        ImVec4 color;
                        const char* name = "?";
                        switch (*it) {
                            case GizmoActivationState::Inactive:
                                color = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
                            name = "I";
                            break;
                            case GizmoActivationState::Probing:
                                color = ImVec4(0.8f, 0.8f, 0.0f, 1.0f);
                            name = "P";
                            break;
                            case GizmoActivationState::Active:
                                color = ImVec4(0.0f, 1.0f, 0.2f, 1.0f);
                            name = "A";
                            break;
                            case GizmoActivationState::Done:
                                color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                            name = "D";
                            break;
                        }

                        // Draw a colored cell
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(color));
                        ImGui::TextUnformatted(name);

                        // Advance the iterator
                        ++it;
                    } else {
                        // If no more values, leave the cell blank
                        ImGui::TextUnformatted("");
                    }
                }
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();

}

#endif

}
