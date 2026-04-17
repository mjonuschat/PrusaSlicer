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

ContextPopup::Callbacks& ContextPopup::callbacks()
{
    return m_callbacks;
}

void ContextPopup::style_node()
{
    if (is_visible()) {
        if (m_open_pos.has_value()) {
            set_left(m_open_pos.value().x());
            set_top(m_open_pos.value().y());
        } else {
            switch (m_position) {
            case Position::Right:
                set_right(-(m_offset + width()));
                set_top(
                    m_flags & ImGuiWindowFlags_ChildMenu ?
                        0 :
                        parent_item()->height() * 0.5f - height() * 0.5f
                );
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
                    m_flags & ImGuiWindowFlags_ChildMenu ?
                        0 :
                        parent_item()->width() * 0.5f - width() * 0.5f
                );
                break;
            }
        }
    }

    Item::style_node();
}

void ContextPopup::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

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
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, m_rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.f, 0.f));

    bool begin = false;
    if (m_flags & ImGuiWindowFlags_ChildMenu) {
        ImGuiID id = ImGui::GetID(object_name().c_str());
        begin      = ImGui::BeginPopupMenuEx(id, object_name().c_str(), m_flags);
    } else {
        begin = ImGui::BeginPopup(object_name().c_str(), m_flags);
    }
    if (begin) {
        m_opened = true;
        if (m_request_close) {
            ImGui::CloseCurrentPopup();
            m_request_close = false;
        }

        render_item_end(pos, size);

        if (m_flags & ImGuiWindowFlags_ChildMenu) {
            ImGui::EndMenu();
        } else {
            ImGui::EndPopup();
        }
    } else if (m_opened) {
        m_opened = false;
        if (m_callbacks.closed) {
            m_callbacks.closed();
        }
    }

    // Revert current paddings and spacing
    ImGui::PopStyleVar(5);
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
        m_open_pos                   = pos;
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
    m_request_close = false;
    if (!m_force_open_popup_in_render) {
        ImGui::OpenPopup(object_name().c_str());
    }
    set_style_dirty();
    if (m_callbacks.opened) {
        m_callbacks.opened();
    }
}

void ContextPopup::open(Vec2f pos)
{
    set_open_pos(pos);
    m_force_open_popup_in_render = true;
    open();
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
