///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ContextPopup.hpp"

#include <Slic3r/App/Yoga/Window.hpp>

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

ContextPopup::ContextPopup(const std::string& name)
{
    set_item_name(name.empty() ? "ContextMenu" : name);
    set_position_type(YGPositionType::YGPositionTypeAbsolute);
}

void ContextPopup::style_node()
{
    if (is_visible()) {
        switch (m_position) {
        case Position::Right:
            set_right(-(m_offset + width()));
            set_top(m_parent->height() * 0.5f - height() * 0.5f);
            break;
        case Position::Left:
            set_left(-(m_offset + width()));
            set_top(m_parent->height() * 0.5f - height() * 0.5f);
            break;
        case Position::Top:
            set_top(-(m_offset + height()));
            set_left(m_parent->width() * 0.5f - width() * 0.5f);
            break;
        case Position::Bottom:
            set_bottom(-(m_offset + height()));
            set_left(m_parent->width() * 0.5f - width() * 0.5f);
            break;
        }
    }

    for (const ItemPtr& item : std::as_const(m_children)) {
        item->style_node();
    }
}

void ContextPopup::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    render_debug(pos, size);

    ImGui::SetNextWindowSize(to_im(size));
    ImGui::SetNextWindowPos(to_im(pos));

    // Discard current paddings and spacing of the window to corect apply of sizer's margins
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, m_rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));

    ImGui::OpenPopupOnItemClick();

    if (ImGui::BeginPopup(m_item_name.c_str(), m_flags)) {
        if (m_request_close) {
            ImGui::CloseCurrentPopup();
            m_request_close = false;
        }

        for (const ItemPtr& child : std::as_const(m_children)) {
            render_node(pos, child.get());
        }

        ImGui::EndPopup();
    }

    // Revert current paddings and spacing
    ImGui::PopStyleVar(4);
}

float ContextPopup::offset() const
{
    return m_offset;
}

void ContextPopup::set_offset(float offset)
{
    if (!Domain::fuzzy_compare(m_offset, offset)) {
        m_offset = offset;
        set_style_dirty();
    }
}

Position ContextPopup::position() const
{
    return m_position;
}

void ContextPopup::set_position(Position position)
{
    if (m_position != position) {
        m_position = position;
        set_top(YGUndefined);
        set_bottom(YGUndefined);
        set_left(YGUndefined);
        set_right(YGUndefined);
        set_style_dirty();
    }
}

float ContextPopup::rounding() const
{
    return m_rounding;
}

void ContextPopup::set_rounding(float rounding)
{
    if (!Domain::fuzzy_compare(m_rounding, rounding)) {
        m_rounding = rounding;
        set_style_dirty();
    }
}

void ContextPopup::open()
{
    m_request_close = false;
    ImGui::OpenPopup(m_item_name.c_str());
    set_style_dirty();
}

void ContextPopup::close()
{
    m_request_close = true;
    set_style_dirty();
}

bool ContextPopup::opened() const
{
    return ImGui::IsPopupOpen(m_item_name.c_str());
}

} // namespace Slic3r::App::Yoga
