///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Oleksandra Iushchenko @YuSanka, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, David Kocík @kocikdav, Filip Sykala @Jony01, Lukáš Hejl @hejllukas, Vojtěch Král @vojtechkral

#include "Slic3r/App/Yoga/Toolbar/Item.hpp"
#include "Slic3r/App/Yoga/Toolbar/Toolbar.hpp"
#include "Slic3r/Log.hpp"

#include <Yoga.h>
#include <string.h>

namespace Slic3r::App::Yoga::Toolbar {

struct ButtonBehaviour {
    bool pressed        { false };
    bool hovered        { false };
    bool pressed_arrow  { false };
    bool hovered_arrow  { false };
};

static ButtonBehaviour CustomRoundedButton(const std::string& label, const ImVec2& pos, const ImVec2& size, bool has_arrow, bool is_toggled, ImDrawFlags rounding_corners/* = ImDrawCornerFlags_None*/)
{
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowBgAlpha(0.f);

    // Discard current paddings and spacing of the window to corect apply of sizer's margins
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

    ImGui::Begin((label + "_win").c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    float rounding{ 4.f };
    ImVec2 button_size = size;
    ImRect button_bb(pos, pos + button_size);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float arrow_h = draw_list->_Data->FontSize * 0.5f;

    ImVec2 arrow_size = ImVec2(1.f, 1.f) * 2.5f * arrow_h;
    ImVec2 arrow_pos = pos + button_size - arrow_size;
    ImRect arrow_bb(arrow_pos, arrow_pos + arrow_size);

    // Check if the arrow is clicked or hovered
    bool hovered_arrow = has_arrow && ImGui::IsMouseHoveringRect(arrow_bb.Min, arrow_bb.Max);
    bool pressed_arrow = has_arrow && hovered_arrow && ImGui::IsMouseClicked(0);

    // Check if the button is clicked or hovered
    bool hovered = ImGui::IsMouseHoveringRect(button_bb.Min, button_bb.Max) && !hovered_arrow;
    bool pressed = hovered && ImGui::IsMouseClicked(0);

    draw_list->AddRectFilled(button_bb.Min, button_bb.Max, ImGui::GetColorU32(ImGuiCol_WindowBg), rounding, rounding_corners);
    button_bb.Expand(-rounding);

    // Draw button background with custom rounding corner(s)
    ImU32 col = ImGui::GetColorU32(hovered ? (is_toggled ? ImGuiCol_WindowBg : ImGuiCol_Button) : (is_toggled ? ImGuiCol_Button : ImGuiCol_WindowBg));
    draw_list->AddRectFilled(button_bb.Min, button_bb.Max, col, rounding, ImDrawFlags_RoundCornersAll);

    // Render the text label in the center of the button
    ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
    ImVec2 text_pos = pos + (button_size - text_size) * 0.5f;
    draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), label.c_str());

    if (has_arrow) {
        // Draw button background with custom rounding on only one corner
        ImU32 arrow_col = ImGui::GetColorU32(hovered_arrow ? ImGuiCol_ButtonHovered : ImGuiCol_Text);

        // draw arrow
        ImVec2 corner_pos = arrow_bb.GetCenter() + ImVec2(1.f, 1.f) * 0.5f * arrow_h;
        draw_list->AddTriangleFilled(corner_pos + ImVec2(0.f, -arrow_h), corner_pos, corner_pos + ImVec2(-arrow_h, 0.f), arrow_col);
    }


    ImGui::End();
    // Revert current paddings and spacing
    ImGui::PopStyleVar(3);

    return { pressed, hovered, pressed_arrow, hovered_arrow };
}

static void Tooltip(const std::string& label, ImVec2 pos, ImVec2 pivot)
{
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowSize(ImVec2(-1.f, 25.f));
    ImGui::SetNextWindowBgAlpha(0.75f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8.f, 8.f });

    ImGui::Begin((label + "_wintt").c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    ImGui::TextUnformatted(label.c_str());
    ImGui::End();

    ImGui::PopStyleVar(2);
}

Item::~Item() 
{
    if (m_has_internal_tollbar && m_sub_toolbar)
        delete m_sub_toolbar;
}

bool Item::is_separator() const
{
    return m_callbacks.is_empty() && !m_sub_toolbar;
}

bool Item::is_visible() const
{
    return m_callbacks.visibility();
}

static Toolbar* active_sub_toolbar  { nullptr };
static ImRect   sub_tb_rect         { ImRect() };

void Item::render_sub_toolbar(ImRect item_bb, ImRect parent_bb, bool force)
{
    static ImVec2   sub_tb_pos{ ImVec2() };
    static ImVec2   sub_tb_size{ ImVec2() };

    if (force && m_sub_toolbar) {
        const float space = 5.f;

        sub_tb_pos = parent_bb.Min + (m_sub_toolbar->is_horizontal() ? ImVec2(0.f, item_bb.GetHeight() + space) : ImVec2(item_bb.GetWidth() + space, 0.f));
        sub_tb_size = m_sub_toolbar->is_horizontal() ? ImVec2(parent_bb.GetWidth(), item_bb.GetHeight()) : ImVec2(item_bb.GetWidth(), parent_bb.GetHeight());

        sub_tb_rect = m_sub_toolbar->get_bb(sub_tb_pos);
        sub_tb_rect.Min -= (m_sub_toolbar->is_horizontal() ? ImVec2(0, 1) : ImVec2(1, 0)) * (space + 10.f);

        active_sub_toolbar = m_sub_toolbar;
    }

    if (active_sub_toolbar && m_sub_toolbar == active_sub_toolbar && (force || ImGui::IsMouseHoveringRect(sub_tb_rect.Min, sub_tb_rect.Max, false)))
        m_sub_toolbar->render(sub_tb_size, sub_tb_pos);

    if (!ImGui::IsMouseHoveringRect(sub_tb_rect.Min, sub_tb_rect.Max, false))
        sub_tb_rect = ImRect(); //invalidate subtoolbar
}

bool Item::render(ImRect item_bb, ImRect parent_bb, ImDrawFlags corners_flag, ImVec2 tooltip_pivot)
{
    const bool has_arrow  = m_callbacks.action_on_arrow || m_sub_toolbar;

    bool is_toggled = m_callbacks.toggled && m_callbacks.toggled();
    if (active_sub_toolbar && m_sub_toolbar == active_sub_toolbar && ImGui::IsMouseHoveringRect(sub_tb_rect.Min, sub_tb_rect.Max, false))
        is_toggled = true;

    // render button

    ButtonBehaviour button_behav = CustomRoundedButton(m_icon_name, item_bb.Min, item_bb.GetSize(), has_arrow, is_toggled, corners_flag);
    // and its sub_tollbar if any exists and have to be shown
    render_sub_toolbar(item_bb, parent_bb, button_behav.hovered || button_behav.pressed_arrow);

    // process callbacks

    if (button_behav.pressed) {
        if (m_callbacks.action)
            m_callbacks.action(item_bb);
    }
    else if (button_behav.pressed_arrow) {
        if (m_callbacks.action_on_arrow)
            m_callbacks.action_on_arrow(item_bb);
    }
    else if (button_behav.hovered_arrow) {
        if (m_callbacks.action_on_arrow_hovering)
            m_callbacks.action_on_arrow_hovering(item_bb);
        else
            render_tooltip(item_bb.Min, item_bb.GetSize(), tooltip_pivot, true);
    }

    return button_behav.hovered &&
           active_sub_toolbar != m_sub_toolbar; // don't show all tooltips, when rendering sub toolbar
}

void Item::render_tooltip(ImVec2 pos, ImVec2 tt_shift /*= ImVec2()*/, ImVec2 pivot, bool for_arrow /*= false*/)
{
    if (m_tooltip.empty())
        return;
    std::string label = m_tooltip + (for_arrow ? " arrow tt" : "");
    Tooltip(label, pos + tt_shift, pivot);
}

void Item::set_sub_toolbar(Toolbar* sub_toolbar)
{
    m_sub_toolbar = sub_toolbar;
}

void Item::init_sub_toolbar(float min_item_size, float max_item_size, Yoga::Align align, Orientation orientation)
{
    if (!m_sub_toolbar) {
        m_sub_toolbar = new Toolbar("sub_" + m_icon_name, min_item_size, max_item_size, align, orientation);
        m_has_internal_tollbar = true;
    }
}

void Item::add_sub_toolbar_item(const Item& item)
{
    assert(m_sub_toolbar);
    m_sub_toolbar->add(item);
}

void Item::insert_sub_toolbar_item(int insert_pos, const Item& item)
{
    assert(m_sub_toolbar);
    m_sub_toolbar->insert(insert_pos, item);
}

void Item::erase_sub_toolbar_item(int erase_pos)
{
    assert(m_sub_toolbar);
    m_sub_toolbar->erase(erase_pos);
}

} // namespace Slic3r::Yoga::Toolbar
