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
    set_object_name(name.empty() ? "ContextMenu" : name);
    set_position_type(YGPositionType::YGPositionTypeAbsolute);
}

void ContextPopup::style_node()
{
    if (is_visible()) {
        if (m_open_pos) {
            set_left((*m_open_pos).x());
            set_top((*m_open_pos).y());
        } else {
            switch (m_position) {
            case Position::Right:
                set_right(-(m_offset + width()));
                set_top(
                    0.f /*parent_item()->height() * 0.5f - height() * 0.5f*/
                ); // #ysFIXME - WIP: need to improve
                break;
            case Position::Left:
                set_left(-(m_offset + width()));
                set_top(parent_item()->height() * 0.5f - height() * 0.5f);
                break;
            case Position::Top:
                set_top(-(m_offset + height()));
                set_left(parent_item()->width() * 0.5f - width() * 0.5f);
                break;
            case Position::Bottom:
                set_bottom(-(m_offset + height()));
                set_left(
                    0.f /*parent_item()->width() * 0.5f - width() * 0.5f*/
                ); // #ysFIXME - WIP: need to improve
                break;
            }
        }
    }

    Item::style_node();
}

void ContextPopup::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    render_debug(pos, size);

    if (m_force_open_popup_in_render) {
        ImGui::OpenPopup(object_name().c_str());
        m_force_open_popup_in_render = false;
    }

    ImGui::SetNextWindowSize(to_im(size));
    ImGui::SetNextWindowPos(to_im(pos));

    // Discard current paddings and spacing of the window to corect apply of sizer's margins
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, m_rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));

    if (ImGui::BeginPopup(object_name().c_str(), m_flags)) {
        if (m_request_close) {
            ImGui::CloseCurrentPopup();
            m_request_close = false;
        }

        for (Item* child : std::as_const(m_children_render_order)) {
            render_node(pos, child);
        }

        ImGui::EndPopup();
    } else if (!m_id_on_right_click.empty()) {
        // Force an OpenPopup on right-click using the given PopupID.
        // Note: (otherwise the last item ID will be used).
        const char* popup_id = m_id_on_right_click.c_str();
        ImGui::OpenPopupOnItemClick(popup_id);

        if (ImGui::BeginPopup(popup_id, m_flags)) {
            for (Item* child : std::as_const(m_children_render_order)) {
                render_node(pos, child);
            }
            ImGui::EndPopup();

            // This action is needed only once,
            // so clear m_id_on_right_click after the first rendering.
            m_id_on_right_click.clear();
        }
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

std::optional<Vec2f> ContextPopup::open_pos() const
{
    return m_open_pos;
}

void ContextPopup::set_open_pos(std::optional<Vec2f> pos)
{
    if (pos != m_open_pos) {
        m_open_pos = pos;
        m_force_open_popup_in_render = true;
        invalidate_style();
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
        invalidate_style();
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
    m_request_close     = false;
    m_id_on_right_click = object_name();
    if (!m_force_open_popup_in_render) {
        ImGui::OpenPopup(object_name().c_str());
    }
    set_style_dirty();
}

void ContextPopup::close()
{
    m_request_close = true;
    set_style_dirty();
}

bool ContextPopup::opened() const
{
    return ImGui::IsPopupOpen(object_name().c_str());
}

ImGuiWindowFlags ContextPopup::flags() const
{
    return m_flags;
}

void ContextPopup::set_flags(ImGuiWindowFlags flags)
{
    m_flags = flags;
}

void ContextPopup::invalidate_style()
{
    set_top(YGUndefined);
    set_bottom(YGUndefined);
    set_left(YGUndefined);
    set_right(YGUndefined);
    set_style_dirty();
}

} // namespace Slic3r::App::Yoga
