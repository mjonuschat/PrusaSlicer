///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Window.hpp"

#include <imgui_internal.h>

#include <utility>

namespace Slic3r::App::Yoga {

std::unordered_map<std::string, int> Window::window_names = {};

Window::Window(const std::string& window_name) : Item()
, m_alpha(GImGui->Style.Alpha)
{
    if (window_names.contains(window_name)) {
        m_window_name = window_name + " " + std::to_string(++window_names[window_name]);
    } else {
        m_window_name = window_name;
        window_names.insert({window_name, 1});
    }
    set_padding(10);
}

const std::string& Window::window_name() const { return m_window_name; }

void Window::set_window_name(const std::string& prefix) { m_window_name = prefix; }

float Window::rounding() const { return m_rounding; }

void Window::set_rounding(float newRounding) { m_rounding = newRounding; }

int Window::flags() const { return m_flags; }

void Window::set_flags(int flags) { m_flags = flags; }

float Window::alpha() const
{
    return m_alpha;
}

void Window::set_alpha(float alpha)
{
    m_alpha = alpha;
}

void Window::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    render_debug(pos, size);

    ImGui::SetNextWindowPos(to_im(pos));
    ImGui::SetNextWindowSize(to_im(size));
    ImGui::SetNextWindowBgAlpha(m_alpha);

    // Discard current paddings and spacing of the window to corect apply of sizer's margins
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, m_rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));

    ImGui::Begin(m_window_name.c_str(), nullptr, m_flags);

    render_body(pos, size);

    for (const ItemPtr& child : std::as_const(m_children)) {
        render_node(pos, child.get());
    }

    ImGui::End();
    // Revert current paddings and spacing
    ImGui::PopStyleVar(4);
}

void Window::render_body(Vec2f pos, Vec2f size) {}

Vec2f Window::get_item_size()
{
    ImVec2 old_pos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos({});

    // render widget with 0 alpha and store their size
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0);
    render({}, {});
    ImGui::PopStyleVar();

    ImVec2 size = ImGui::GetCursorScreenPos();

    Vec2f result = Vec2f{ImMax(0.f, size.x), ImMax(0.f, size.y)};

    // reset cursor pos
    ImGui::SetCursorScreenPos(old_pos);

    return result;
}

} // namespace Slic3r::App::Yoga
