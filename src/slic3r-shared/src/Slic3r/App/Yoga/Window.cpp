///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Window.hpp"

#include <imgui_internal.h>

#include <utility>

namespace Slic3r::App::Yoga {

Window::Window(const std::string& window_name) : Item(), m_alpha(GImGui->Style.Alpha)
{
    set_item_name(window_name);
    set_padding(10);
}

float Window::rounding() const
{
    return m_rounding;
}

void Window::set_rounding(float newRounding)
{
    m_rounding = newRounding;
}

int Window::flags() const
{
    return m_flags;
}

void Window::set_flags(int flags)
{
    m_flags = flags;
}

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

    if (m_position_by_yoga) {
        ImGui::SetNextWindowPos(to_im(pos));
    } else if (m_requested_position.has_value()) {
        ImGui::SetNextWindowPos(to_im(m_requested_position.value()));
        m_requested_position = {};
    }
    ImGui::SetNextWindowSize(to_im(size));
    ImGui::SetNextWindowBgAlpha(m_alpha);

    // Discard current paddings and spacing of the window to corect apply of sizer's margins
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, m_rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));

    ImGui::Begin(m_item_name.c_str(), nullptr, m_flags);

    ImVec2 pos_to_render = ImGui::GetWindowPos();

    if (!m_position_by_yoga) {
        ImVec2 xy  = pos_to_render;
        m_last_pos = from_im(xy);
        ImVec2 sz  = ImGui::GetWindowSize();
        ImVec2 max = ImGui::GetMainViewport()->Size - sz;
        ImVec2 pos = {std::clamp(xy.x, 0.f, max.x), std::clamp(xy.y, 0.f, max.y)};
        ImGui::SetWindowPos(pos);
    }

    Vec2f window_pos = from_im(pos_to_render);

    render_body(window_pos, size);

    for (const ItemPtr& child : std::as_const(m_children)) {
        render_node(window_pos, child.get());
    }

    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    if (m_hovered != hovered) {
        m_hovered = hovered;
        if (m_callbacks.hovered_changed) {
            m_callbacks.hovered_changed(m_hovered);
        }
    }

    if (m_requested_bring_to_front) {
        m_requested_bring_to_front = false;
        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    }

    ImGui::End();
    // Revert current paddings and spacing
    ImGui::PopStyleVar(4);
}

void Window::render_body(Vec2f pos, Vec2f size) {}

void Window::process_events(Vec2f pos, Vec2f size)
{
    Item::process_events(m_position_by_yoga ? pos : m_last_pos, size);
}

void Window::set_style_dirty()
{
    if (m_parent) {
        m_parent->set_style_dirty();
    } else if (m_callbacks.set_dirty_requested) {
        m_callbacks.set_dirty_requested();
    }
}

Window::Callbacks& Window::callbacks()
{
    return m_callbacks;
}

bool Window::position_by_yoga() const
{
    return m_position_by_yoga;
}

void Window::set_position_by_yoga(bool position_by_yoga)
{
    m_position_by_yoga = position_by_yoga;
}

void Window::request_position(Vec2f position)
{
    m_requested_position = position;
}

void Window::bring_to_front()
{
    m_requested_bring_to_front = true;
    set_style_dirty();
}

bool Window::hovered() const
{
    return m_hovered;
}

} // namespace Slic3r::App::Yoga
