#include "Slic3r/App/Yoga/Tooltip.hpp"

#include "imgui_internal.h"

namespace Slic3r::App::Yoga {

Tooltip::Tooltip(
    const std::string& window_name, const std::string& text, const std::string& shortcut, Item* parent
)
    : Window(window_name, parent), m_text(text), m_shortcut(shortcut)
{
    set_flags(
        ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove
    );
}

void Tooltip::render_body(Vec2f pos, Vec2f size)
{
    ImGui::TextUnformatted(m_text.c_str());
    if (!m_shortcut.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(GImGui->Style.Colors[ImGuiCol_TextDisabled], "%s", m_shortcut.c_str());
    }
}

void Tooltip::style_node()
{
    Window::style_node();
    // make some magic here
}

const std::string& Tooltip::text() const { return m_text; }

void Tooltip::set_text(const std::string& text) { m_text = text; }

const std::string& Tooltip::shortcut() const { return m_shortcut; }

void Tooltip::set_shortcut(const std::string& shortcut) { m_shortcut = shortcut; }

} // namespace Slic3r::App::Yoga
