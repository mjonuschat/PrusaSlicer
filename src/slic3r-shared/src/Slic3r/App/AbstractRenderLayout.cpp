#include "Slic3r/App/AbstractRenderLayout.hpp"
#include "Slic3r/Log.hpp"

#include <Yoga.h>
#include <imgui_internal.h>

namespace Slic3r::App {

using Vec2f = Yoga::Vec2f;

Vec2f AbstractRenderLayout::win_padding()
{
    return Vec2f(GImGui->Style.WindowPadding.x, GImGui->Style.WindowPadding.y);
}

Vec2f AbstractRenderLayout::frame_padding()
{
    return Vec2f(GImGui->Style.FramePadding.x, GImGui->Style.FramePadding.y);
}

void AbstractRenderLayout::init_main_sizer()
{
#if MAIN_WITH_SPLITTERS
    m_main_sizer.init(3);
    m_main_sizer.set_splitter_padding(0.f);
    m_main_sizer.set_splitter_sz(2.f);
    m_main_sizer.show_splitter(false);
#else
    m_main_sizer.init(3, 1, Vec2f(0.f, 0.f), Yoga::Margins(win_padding() * 0.5f));
#endif
    m_main_sizer.set_grow_col(1);

    init_view_cube_sizer();
    init_left_sizer();
    init_middle_sizer();
    init_right_sizer();

    m_main_sizer.add(left_sizer);
    m_main_sizer.add(middle_sizer);
    m_main_sizer.add(right_sizer);
}

void AbstractRenderLayout::add_panel(Yoga::FlexSizer& sizer, std::function<void(Vec2f, Vec2f)> render_item_fn, std::string win_name, Vec2f win_paddings /*= { -1.f, -1.f }*/)
{
    if (win_paddings.x() < 0.f || win_paddings.y() < 0.f)
        win_paddings = frame_padding() * 4.f;

    sizer.add(render_item_fn, false, { win_name, win_paddings });
}

const static float min_tt_size = 50.f;// 30.f;
const static float max_tt_size = 50.f;

void AbstractRenderLayout::init_view_cube_sizer()
{
    static Yoga::FlexSizer sizer_in(1, 1);
    sizer_in.set_bg_alpha(0.f);
    sizer_in.add(m_cb_cube_view_render, true, { "view_cube" });

    view_cube_sizer.init(1, 1);
    view_cube_sizer.set_grow_col(0);
    view_cube_sizer.set_grow_row(0, 0.f);
    view_cube_sizer.add(sizer_in, {}, { Yoga::AlignH::Right, Yoga::AlignV::Top });
}

void AbstractRenderLayout::init_toolbars_sizer()
{
    // Just "sceleton" for the toolbars is created here
    // All render functions are empty, because of no one item is added to the toolbar jet
    // So, as a workaround, lets render button with max_tt_size and with 0 alpha
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0);
    ImGui::Button("win##workaround", ImVec2(max_tt_size, max_tt_size));
    ImGui::PopStyleVar();

    top_toolbar.init("top_toolbar", min_tt_size, max_tt_size, { Yoga::AlignH::Left, Yoga::AlignV::Top });

    middle_toolbar.init("middle_toolbar", min_tt_size, max_tt_size, { Yoga::AlignH::Left, Yoga::AlignV::Center }, FlexToolbarOrientation::Vertical);
    middle_toolbar.collapse_if_needed();

    bottom_toolbar.init("bottom_toolbar", min_tt_size, max_tt_size, { Yoga::AlignH::Left, Yoga::AlignV::Bottom });

    m_toolbars_sizer.init(1, 3);
    m_toolbars_sizer.set_grow_row(0, 0.f);
    m_toolbars_sizer.set_grow_row(2, 0.f);

    m_toolbars_sizer.add([this](Vec2f size, Vec2f pos) {
        top_toolbar.render(size, pos);
    }, true);
    m_toolbars_sizer.add([this](Vec2f size, Vec2f pos) {
        middle_toolbar.render(size, pos);
    }, true);
    m_toolbars_sizer.add([this](Vec2f size, Vec2f pos) {
        bottom_toolbar.render(size, pos);
    }, true);
}

void AbstractRenderLayout::layout_toolbars_sizer()
{
    m_toolbars_sizer.set_grow_row(0, float(top_toolbar.shown_items_cnt()));
    m_toolbars_sizer.set_grow_row(1, float(middle_toolbar.shown_items_cnt()));
    m_toolbars_sizer.set_grow_row(2, float(bottom_toolbar.shown_items_cnt()));
    m_toolbars_sizer.layout();
}

void AbstractRenderLayout::add_middle_flex_sizer()
{
    middle_sizer.add(view_cube_sizer);
}

void AbstractRenderLayout::init_middle_sizer()
{
    if (!m_toolbars_sizer.is_inited())
        init_toolbars_sizer();

    middle_sizer.init(2, 1, Vec2f(0.f, 0.f), Yoga::Margins(win_padding() * 0.5f));
    middle_sizer.set_grow_col(1);
    middle_sizer.add(m_toolbars_sizer);
    add_middle_flex_sizer();
}

void AbstractRenderLayout::add_toolbar_item(ToolbarID id, wchar_t icon, const std::string& tooltip, const std::string& shortcut, Yoga::Toolbar::Callbacks callbacks)
{
    if (!m_toolbars_sizer.is_inited())
        init_toolbars_sizer();

    FlexToolbar& toolbar = id == ToolbarID::Top     ? top_toolbar :
                           id == ToolbarID::Middle  ? middle_toolbar : bottom_toolbar;

    toolbar.add(icon, tooltip, shortcut, callbacks);
    layout_toolbars_sizer();
}

void AbstractRenderLayout::add_toolbar_separator(ToolbarID id, float size)
{
    assert(m_toolbars_sizer.is_inited());

    FlexToolbar& toolbar = id == ToolbarID::Top     ? top_toolbar :
                           id == ToolbarID::Middle  ? middle_toolbar : bottom_toolbar;

    toolbar.add_separator(size < 0.f ? win_padding().y() : size);
    layout_toolbars_sizer();
}

void AbstractRenderLayout::show_left(int panel_id, bool show)
{
    left_sizer.show_row(panel_id, show);

    bool is_any_visible{ false };
    for (size_t id = 0; id < left_sizer.get_rows(); id++) {
        if (left_sizer.is_shown_row(id)) {
            is_any_visible = true;
            break;
        }
    }

    if (m_main_sizer.is_shown_col(0) != is_any_visible)
        m_main_sizer.show_col(0, is_any_visible);
}

void AbstractRenderLayout::show_right(int panel_id, bool show)
{
    right_sizer.show_row(panel_id, show);

    bool is_any_visible{ false };
    for (size_t id = 0; id < right_sizer.get_rows(); id++) {
        if (right_sizer.is_shown_row(id)) {
            is_any_visible = true;
            break;
        }
    }

    if (m_main_sizer.is_shown_col(2) != is_any_visible)
        m_main_sizer.show_col(2, is_any_visible);
}

static void SetOurStyleColors()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.168f, 0.168f, 0.168f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.21f, 0.29f, 0.46f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.21f, 0.29f, 0.46f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.21f, 0.29f, 0.46f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected]            = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline]    = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed]              = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected]      = ImLerp(colors[ImGuiCol_TabSelected],  colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink]               = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

struct SetOurStyleVars
{
    SetOurStyleVars() {
        PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
        PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 0.f));
        PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 6.f));
        PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.f, 0.f));
    }
        
    ~SetOurStyleVars() {
        ImGui::PopStyleVar(m_vars_cnt);
    }

private:
    void PushStyleVar(ImGuiStyleVar idx, float val) {
        ImGui::PushStyleVar(idx, val);
        m_vars_cnt++;
    }
    void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val) {
        ImGui::PushStyleVar(idx, val);
        m_vars_cnt++;
    }

    size_t m_vars_cnt{ 0 };
};

void AbstractRenderLayout::render(Vec2f size)
{
    if (!m_main_sizer.is_inited())
        init_main_sizer();

    SetOurStyleColors();
    {
        SetOurStyleVars our_vars;
        m_main_sizer.render(size, Vec2f(0.f, 0.f));
    }
}

}
