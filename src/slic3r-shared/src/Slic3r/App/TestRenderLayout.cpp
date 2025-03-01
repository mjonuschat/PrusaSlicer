#include "Slic3r/App/TestRenderLayout.hpp"
#include "Slic3r/Log.hpp"

#include <Yoga.h>
#include "Slic3r/App/Yoga/SplitterSizer.hpp"

#include <imgui/imgui_internal.h>

namespace Slic3r::App {

#define render_main_window 0

using namespace Slic3r::App::Yoga;

static bool combo(const std::string& label, const std::vector<std::string>& options, ImGuiComboFlags flags = 0, float label_width = 0.0f, float item_width = 100.0f)
{
    ImGui::PushItemWidth(item_width);

    static int selection = 0;
    int selection_out = selection;
    bool res = false;

    const char* selection_str = selection < int(options.size()) && selection >= 0 ? options[selection].c_str() : "";
    if (ImGui::BeginCombo(("##" + label).c_str(), selection_str, flags)) {
        for (int i = 0; i < (int)options.size(); i++) {
            if (ImGui::Selectable(options[i].c_str(), i == selection)) {
                selection_out = i;
                res = true;
            }
        }
        ImGui::EndCombo();
    }

    selection = selection_out;
    return res;
}

void TestRenderLayout::init_main_sizer()
{
#if flex_with_splitters
    m_main_sizer.init(3);
    m_main_sizer.set_splitter_padding(2.f);
#else
    m_main_sizer.init(3, 1);
#endif
    m_main_sizer.set_bg_alpha(1.f);
    m_main_sizer.set_grow_col(1);

    init_left_sizer();
    init_middle_sizer();
    init_right_sizer();

#if render_main_window
    m_main_sizer.add(m_left_sizer);
#else
    m_main_sizer.add(m_left_sizer, "left");
#endif
    m_main_sizer.add(m_middle_sizer);
    m_main_sizer.add(m_right_sizer);
} 

void TestRenderLayout::init_left_sizer()
{
    m_left_sizer.init(1, ImVec2(), false);
    m_left_sizer.set_grow_row(0);

    m_left_sizer.add([this](ImVec2 size, ImVec2 pos) {
        m_cb_object_list_render(size, pos);
    }, { AlignH::Left, AlignV::Top }, "object_list");
}

void TestRenderLayout::init_middle_sizer()
{
    m_middle_sizer.initialize();

    // add items with callbacks for top corner toolbars

    // callbacks for toolbar items
    auto cb_is_visible      = []() -> bool {return true; };
    auto cb_is_enable       = []() -> bool {return true; };
    auto show_hide_panel    = [this](bool& show, int panel_id) -> void {
        show = !show;
        m_main_sizer.show_col(panel_id, show);
    };

    static bool show_left   { true };
    static bool show_right  { true };

    // create top left toolbar, which contains just one item
    m_middle_sizer.top_left_toolbar.add("S/H L", "Show/Hide left panel",   { [show_hide_panel](ImRect) { show_hide_panel(show_left, 0); }, cb_is_visible, cb_is_enable, []() { return !show_left; } });

    // create top right toolbar, which contains just one item
    m_middle_sizer.top_right_toolbar.add("S/H R", "Show/Hide right panel", { [show_hide_panel](ImRect) { show_hide_panel(show_right, 2); }, cb_is_visible, cb_is_enable, []() { return !show_right; } });
}

void TestRenderLayout::init_right_sizer()
{
    m_right_sizer.init(2, ImVec2(), false);
    m_right_sizer.set_grow_row(1);
    m_right_sizer.set_bg_alpha(0.35f);
    m_right_sizer.set_splitter_padding(3.f);

    static FlexSizer right_top_sizer(2, 2, ImVec2(), ImVec2(5.f, 15.f)); 
    right_top_sizer.set_grow_col(1);
    right_top_sizer.set_grow_row(0, 0.f);
    right_top_sizer.set_grow_row(1, 0.f);

    static FlexSizer right_bottom_sizer(2, 1); // top right bottom 
    right_bottom_sizer.set_grow_col(1);

    static FlexSizer right_buttons_sizer(2, 1, ImVec2(0.f, 0.f), ImVec2(0.f, 0.f));
    right_buttons_sizer.set_grow_col(1);

    right_top_sizer.add([](ImVec2, ImVec2) { ImGui::Text("Horizontal Align:"); }, { AlignH::Right });
    right_top_sizer.add([](ImVec2 size, ImVec2) { combo("##horiz", { "Left", "Center", "Right" }, 0, 0.f, size.x == 0.f ? 100.f : size.x); });

    right_top_sizer.add([](ImVec2, ImVec2) { ImGui::Text("Vertical Align:"); }, { AlignH::Right });
    right_top_sizer.add([](ImVec2 size, ImVec2) { combo("##vert", { "Top", "Center", "Bottom" }, 0, 0.f, size.x == 0.f ? 100.f : size.x); });

    right_buttons_sizer.add([](ImVec2, ImVec2) {
        if (ImGui::Button("Align Column")) {
        }
        ImGui::SameLine();
        if (ImGui::Button("Align Row")) {
        }
    });

    right_buttons_sizer.add([](ImVec2, ImVec2) {
        if (ImGui::Button("Align Cell")) {
        }
    }, { AlignH::Right, AlignV::Bottom });

    right_bottom_sizer.add(nullptr);
    right_bottom_sizer.add(right_buttons_sizer);
#if render_main_window
    m_right_sizer.add(right_top_sizer);
    m_right_sizer.add(right_bottom_sizer);
#else
    m_right_sizer.add(right_top_sizer, "right_top");
    m_right_sizer.add(right_bottom_sizer, "right_bottom");
#endif
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
        PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f, 4.f));
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

void TestRenderLayout::render(ImVec2 size)
{
    if (!m_main_sizer.is_init())
        init_main_sizer();

    ImVec2 sizer_size = size - GImGui->Style.WindowPadding * 2.f;
    ImVec2 sizer_pos  = GImGui->Style.WindowPadding;

    SetOurStyleColors();
    {
        SetOurStyleVars our_vars;
        m_main_sizer.render(sizer_size, sizer_pos);
    }

}

}
