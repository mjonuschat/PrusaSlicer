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

float Window::rounding() const { return m_rounding; }

void Window::set_rounding(float newRounding) { m_rounding = newRounding; }

int Window::flags() const { return m_flags; }

void Window::set_flags(int flags) { m_flags = flags; }

float Window::alpha() const { return m_alpha; }

void Window::set_alpha(float alpha) { m_alpha = alpha; }

void Window::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    render_debug(pos, size);

    ImVec2 next_pos;
    if (m_position_by_yoga || !m_requested_position.has_value()) {
        next_pos = to_im(pos);
    } else if (m_requested_position.has_value()) {
        next_pos = to_im(m_requested_position.value());
        m_requested_position = {};
    }

    ImVec2 sz = to_im(size);
    if (!m_position_by_yoga) {
        // clamp
        ImVec2 vp = ImGui::GetMainViewport()->Size;
        ImVec2 max = ImVec2(vp.x - sz.x, vp.y - sz.y);
        next_pos.x = std::clamp(next_pos.x, 0.f, std::max(0.f, max.x));
        next_pos.y = std::clamp(next_pos.y, 0.f, std::max(0.f, max.y));
    }


    ImGui::SetNextWindowSize(sz);
    ImGui::SetNextWindowBgAlpha(m_alpha);
    ImGui::SetNextWindowPos(next_pos);

    // Discard current paddings and spacing of the window to corect apply of sizer's margins
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, m_rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));

    ImGui::Begin(m_item_name.c_str(), nullptr, m_flags);

    m_last_pos = from_im(ImGui::GetWindowPos());

    render_body(m_last_pos, size);

    for (const ItemPtr& child : std::as_const(m_children)) {
        render_node(m_last_pos, child.get());
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

bool Window::position_by_yoga() const { return m_position_by_yoga; }

void Window::set_position_by_yoga(bool position_by_yoga) { m_position_by_yoga = position_by_yoga; }

void Window::request_position(Vec2f position) { m_requested_position = position; }

} // namespace Slic3r::App::Yoga
