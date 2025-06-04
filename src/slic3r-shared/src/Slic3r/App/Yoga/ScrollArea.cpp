///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ScrollArea.hpp"

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

ScrollArea::ScrollArea(const std::string& name) : Item()
{
    static int scroll_area_counter = 1;
    set_item_name((name.empty() ? "ScrollArea" : name) + " " + std::to_string(scroll_area_counter++));
}

void ScrollArea::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    render_debug(pos, size);

    ImGui::SetNextWindowPos(to_im(pos));
    ImGui::SetNextWindowSize(to_im(size));
    ImGui::BeginChild(item_name().c_str(), {size.x(), size.y()}, m_child_flags, m_window_flags);

    pos -= Vec2f{ImGui::GetScrollX(), ImGui::GetScrollY()};
    for (Item* child : std::as_const(m_children_render_order)) {
        render_node(pos, child);
    }

    ImGui::EndChild();
}

ImGuiChildFlags ScrollArea::child_flags() const { return m_child_flags; }

void ScrollArea::set_child_flags(ImGuiChildFlags child_flags) { m_child_flags = child_flags; }

ImGuiWindowFlags ScrollArea::window_flags() const { return m_window_flags; }

void ScrollArea::set_window_flags(ImGuiWindowFlags window_flags) { m_window_flags = window_flags; }

} // namespace Slic3r::App::Yoga
