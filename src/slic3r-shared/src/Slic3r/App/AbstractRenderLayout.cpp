#include "Slic3r/App/AbstractRenderLayout.hpp"

#include <Yoga.h>
#include <imgui_internal.h>

#include "Slic3r/App/CubeView.hpp"
#include "Slic3r/App/ObjectList.hpp"
#include "Slic3r/App/SidebarBed.hpp"
#include "Slic3r/App/SidebarPrint.hpp"
#include "Slic3r/App/Yoga/Toolbar.hpp"
#include "Slic3r/App/Yoga/IconButton.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

namespace Slic3r::App {

using Vec2f = Yoga::Vec2f;

Vec2f AbstractRenderLayout::win_padding() const
{
    return Vec2f(GImGui->Style.WindowPadding.x, GImGui->Style.WindowPadding.y);
}

Vec2f AbstractRenderLayout::frame_padding() const
{
    return Vec2f(GImGui->Style.FramePadding.x, GImGui->Style.FramePadding.y);
}

void AbstractRenderLayout::add_toolbar_item(
    ToolbarID id,
    wchar_t icon,
    const std::string& tooltip,
    const std::string& shortcut,
    Yoga::AbstractButton::Callbacks callbacks
)
{
    Yoga::Toolbar* toolbar = nullptr;
    switch (id) {
    case ToolbarID::Top:
        toolbar = m_top_toolbar;
        break;
    case ToolbarID::Middle:
        toolbar = m_middle_toolbar;
        break;
    case ToolbarID::Bottom:
        toolbar = m_bottom_toolbar;
        break;
    }

    ASSERT(toolbar);

    Yoga::IconButton* button = new Yoga::IconButton(icon, tooltip);
    button->callbacks() = callbacks;
    toolbar->append(button);
}

Yoga::Toolbar* AbstractRenderLayout::bottom_toolbar() const { return m_bottom_toolbar; }

Yoga::Toolbar* AbstractRenderLayout::middle_toolbar() const { return m_middle_toolbar; }

Yoga::Toolbar* AbstractRenderLayout::top_toolbar() const { return m_top_toolbar; }

void AbstractRenderLayout::init()
{
    m_layout_main.set_gap(5);
    m_layout_main.set_orientation(Yoga::Orientation::Horizontal);
    m_layout_main.set_padding(Yoga::Paddings(frame_padding()));
    m_layout_main.set_flex_grow(1.0);

    init_left_column();

    init_middle_column();

    init_right_column();
}

void AbstractRenderLayout::init_left_column()
{
    m_layout_left_column = new Yoga::Item;
    m_layout_left_column->set_orientation(Yoga::Orientation::Vertical);
    m_layout_left_column->set_gap(5);
    m_layout_main.append(m_layout_left_column);

    m_object_list->set_flex_grow(1.);
    m_layout_left_column->append(m_object_list);
}

void AbstractRenderLayout::init_middle_column()
{
    m_layout_center_row = new Yoga::Item;
    m_layout_center_row->set_orientation(Yoga::Orientation::Horizontal);
    m_layout_center_row->set_gap(5);
    m_layout_center_row->set_flex_grow(1.);

    m_layout_main.append(m_layout_center_row);

    init_toolbar_column();

    m_layout_middle_column = new Yoga::Item;
    m_layout_middle_column->set_orientation(Yoga::Orientation::Vertical);
    m_layout_middle_column->set_gap(5);
    m_layout_middle_column->set_flex_grow(1);
    m_layout_center_row->append(m_layout_middle_column);

    m_layout_middle_column->append(m_cube_view);
    m_cube_view->set_self_align(YGAlignFlexEnd);
}

void AbstractRenderLayout::init_right_column()
{
    m_layout_right_column = new Yoga::Item;
    m_layout_right_column->set_orientation(Yoga::Orientation::Vertical);
    m_layout_right_column->set_gap(5);
    m_layout_right_column->set_min_size({280.f, 0});

    m_layout_right_column->append(m_sidebar_bed);

    m_sidebar_print->set_flex_grow(1.0);
    m_layout_right_column->append(m_sidebar_print);

    m_layout_main.append(m_layout_right_column);
}

void AbstractRenderLayout::init_toolbar_column()
{
    constexpr float min_tt_size = 50.f; //**/ 30.f;
    constexpr float max_tt_size = 50.f;

    m_layout_left_toolbar_column = new Yoga::Item;
    m_layout_left_toolbar_column->set_orientation(Yoga::Orientation::Vertical);
    m_layout_left_toolbar_column->set_gap(5);
    m_layout_left_toolbar_column->set_justify_content(YGJustify::YGJustifySpaceBetween);

    m_layout_center_row->append(m_layout_left_toolbar_column);

    m_top_toolbar = new Yoga::Toolbar("top_toolbar");
    m_top_toolbar->set_button_min_size({min_tt_size, min_tt_size});
    m_top_toolbar->set_button_max_size({max_tt_size, max_tt_size});
    m_top_toolbar->set_self_align(YGAlign::YGAlignFlexStart);
    m_top_toolbar->set_orientation(Yoga::Orientation::Vertical);
    m_layout_left_toolbar_column->append(m_top_toolbar);

    m_middle_toolbar = new Yoga::Toolbar("middle_toolbar");
    m_middle_toolbar->set_button_min_size({min_tt_size, min_tt_size});
    m_middle_toolbar->set_button_max_size({max_tt_size, max_tt_size});
    m_middle_toolbar->set_self_align(YGAlign::YGAlignCenter);
    m_middle_toolbar->set_orientation(Yoga::Orientation::Vertical);
    m_layout_left_toolbar_column->append(m_middle_toolbar);
    // TODO: set collapsible middle toolbar

    m_bottom_toolbar = new Yoga::Toolbar("bottom_toolbar");
    m_bottom_toolbar->set_button_min_size({min_tt_size, min_tt_size});
    m_bottom_toolbar->set_button_max_size({max_tt_size, max_tt_size});
    m_bottom_toolbar->set_self_align(YGAlign::YGAlignFlexEnd);
    m_bottom_toolbar->set_orientation(Yoga::Orientation::Vertical);
    m_layout_left_toolbar_column->append(m_bottom_toolbar);
}

static void SetOurStyleColors()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.168f, 0.168f, 0.168f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.21f, 0.29f, 0.46f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.21f, 0.29f, 0.46f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.21f, 0.29f, 0.46f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected] =
        ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected] =
        ImLerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] =
        ImVec4(0.31f, 0.31f, 0.35f, 1.00f); // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight] =
        ImVec4(0.23f, 0.23f, 0.25f, 1.00f); // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

struct SetOurStyleVars
{
    SetOurStyleVars()
    {
        PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
        PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));
        PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 6.f));
        PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.f, 0.f));
    }

    ~SetOurStyleVars() { ImGui::PopStyleVar(m_vars_cnt); }

private:
    void PushStyleVar(ImGuiStyleVar idx, float val)
    {
        ImGui::PushStyleVar(idx, val);
        m_vars_cnt++;
    }
    void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val)
    {
        ImGui::PushStyleVar(idx, val);
        m_vars_cnt++;
    }

    size_t m_vars_cnt{0};
};

AbstractRenderLayout::AbstractRenderLayout(
    ObjectList* object_list,
    CubeView* cube_view,
    SidebarBed* sidebar_bed,
    SidebarPrint* sidebar_print
)
    : m_object_list(object_list)
    , m_cube_view(cube_view)
    , m_sidebar_bed(sidebar_bed)
    , m_sidebar_print(sidebar_print)
{}

AbstractRenderLayout::~AbstractRenderLayout() {}

void AbstractRenderLayout::render(Vec2f size)
{
    SetOurStyleColors();
    {
        SetOurStyleVars our_vars;
        m_layout_main.render({}, size);
        // ImGui::DebugStartItemPicker();
        // std::string tree = m_layout_main.debug_dump_tree();
        // ImGui::SetClipboardText(tree.c_str());
    }
}

} // namespace Slic3r::App
