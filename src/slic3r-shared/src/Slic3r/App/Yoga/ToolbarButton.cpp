///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ToolbarButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"
#include "Slic3r/App/Yoga/Toolbar.hpp"
#include "Slic3r/App/Yoga/Dialog.hpp"
#include "Slic3r/Assert.hpp"

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

ToolbarButton::ToolbarButton(Render::Icon icon, const std::string& tooltip)
    : LayoutButton("", icon, tooltip)
{
    set_background_color(ImGui::GetColorU32(ImGuiCol_WindowBg));
    set_background_color_checked(ImColor(60, 60, 60));
}

void ToolbarButton::render(Vec2f pos, Vec2f size)
{
    // render_item_begin(pos, size);
    // render_item_end(pos, size);
    LayoutButton::render(pos, size);

    // Abstract button may have closed the Tooltip
    if (m_tooltip_open && !m_tooltip.opened()) {
        m_tooltip.open();
    }

    // render arrow on top of the children
    if (has_arrow()) {
        ImVec2 button_size = to_im(size);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const float arrow_h = draw_list->_Data->FontSize * 0.35f;

        ImVec2 arrow_size(arrow_h, arrow_h);
        ImVec2 arrow_pos = to_im(pos) + button_size - arrow_size;
        ImRect arrow_bb(arrow_pos, arrow_pos + arrow_size);

        // Draw button background with custom rounding on only one corner
        ImU32 arrow_col = ImGui::GetColorU32(ImGuiCol_Text);

        // draw arrow
        ImVec2 corner_pos = arrow_bb.GetCenter() + ImVec2(1.f, 1.f) * 0.5f * arrow_h;
        draw_list->AddTriangleFilled(
            corner_pos + ImVec2(0.f, -arrow_h), corner_pos, corner_pos + ImVec2(-arrow_h, 0.f),
            arrow_col
        );
    }
}

void ToolbarButton::style_node()
{
    constexpr float gap = 10;

    if (m_subtoolbar) {
        ASSERT(m_parent);
        if (m_parent->orientation() == Yoga::Orientation::Vertical) {
            const float our_height = height();
            const float toolbar_height = m_subtoolbar->height();

            const float top = our_height * 0.5 - toolbar_height * 0.5;

            m_subtoolbar->set_right(-width() - gap);
            m_subtoolbar->set_top(top);
        } else {
            const float our_width = width();
            const float toolbar_width = m_subtoolbar->width();

            const float left = our_width * 0.5 - toolbar_width * 0.5;

            m_subtoolbar->set_left(left);
            m_subtoolbar->set_top(height() + gap);
        }
    }

    Toolbar* parent_toolbar = dynamic_cast<Toolbar*>(m_parent);
    bool new_tooltip_open = parent_toolbar && parent_toolbar->show_tooltips() && is_visible();
    if (new_tooltip_open != m_tooltip_open) {
        m_tooltip_open = new_tooltip_open;
        set_style_dirty();
    }
    if (m_tooltip_open) {
        m_tooltip.open();
    } else {
        m_tooltip.close();
    }

    AbstractButton::style_node();
}

Toolbar* ToolbarButton::get_subtoolbar() const { return m_subtoolbar; }

Toolbar* ToolbarButton::get_or_create_subtoolbar()
{
    if (!m_subtoolbar) {
        m_subtoolbar = emplace_back<Toolbar>("subtoolbar");
        m_subtoolbar->set_orientation(Orientation::Vertical);
        m_subtoolbar->set_position_type(YGPositionType::YGPositionTypeAbsolute);
        m_subtoolbar->set_button_aspect_ratio(m_aspect_ratio);
        m_subtoolbar->set_button_min_size(m_min_size);
        m_subtoolbar->set_button_max_size(m_max_size);
        m_subtoolbar->set_visible(false);

        callbacks().action = [this] {
            if (m_subtoolbar) {
                m_subtoolbar->set_visible(!m_subtoolbar->is_visible());

                Toolbar* parent_toolbar = dynamic_cast<Toolbar*>(m_parent);
                if (parent_toolbar && parent_toolbar->callbacks().subtoolbar_opened) {
                    parent_toolbar->callbacks().subtoolbar_opened();
                }
            }
        };
    }

    return m_subtoolbar;
}

} // namespace Slic3r::App::Yoga
