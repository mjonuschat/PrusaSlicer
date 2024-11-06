#include "Slic3r/App/TestRenderLayout.hpp"
#include "Slic3r/Log.hpp"

#include <Yoga.h>
#include "Slic3r/App/Yoga/SplitterSizer.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
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
    m_main_sizer.set_bg_alpha(0.45f);
#else
    m_main_sizer.init(3, 1);
    m_main_sizer.set_bg_alpha(0.25f);
#endif
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
    m_left_sizer.init(2, ImVec2(), false);
    m_left_sizer.set_grow_row(0);

    static FlexSizer left_top_sizer(2, 2, ImVec2(), ImVec2(5.f, 10.f));
    left_top_sizer.set_grow_col(1);
    left_top_sizer.set_grow_row(0, 0.f);
    left_top_sizer.set_grow_row(1, 0.f);

    left_top_sizer.add([](ImVec2, ImVec2) { ImGui::Text("Column:"); }, { AlignH::Right });
    left_top_sizer.add([](ImVec2, ImVec2) { combo("##column", { "1", "2", "3" }); });

    left_top_sizer.add([](ImVec2, ImVec2) { ImGui::Text("Row:"); }, { AlignH::Right });
    left_top_sizer.add([](ImVec2, ImVec2) { combo("##rows", { "1", "2", "3", "4", "5", "6", "7", "8", "9" }); });

    static FlexSizer left_bottom_sizer(2, 1, ImVec2(), ImVec2(5.f, 10.f));
    left_bottom_sizer.set_grow_col(1);

    left_bottom_sizer.add([](ImVec2, ImVec2) {
        static float grow = 1;
        ImGui::PushItemWidth(100);
        ImGui::InputFloat("Grow", &grow, 1.f, 5.f, "%.1f");
        ImGui::PopItemWidth();
    });

    static FlexSizer left_buttons_sizer(2, 1, ImVec2(0.f, 0.f), ImVec2(2.f, 0.f));
    left_buttons_sizer.set_grow_col(0);
    left_buttons_sizer.set_grow_col(1);

    left_buttons_sizer.add([](ImVec2, ImVec2) {
        if (ImGui::Button("Set Flex Column")) {
        }
    }, { AlignH::Right, AlignV::Bottom });

    left_buttons_sizer.add([](ImVec2, ImVec2) {
        if (ImGui::Button("Set Flex Row")) {
        }
    }, { AlignH::Right, AlignV::Bottom });

    left_bottom_sizer.add(left_buttons_sizer);

    m_left_sizer.add(left_top_sizer);
    m_left_sizer.add(left_bottom_sizer);
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

void TestRenderLayout::render(ImVec2 size)
{
    if (!m_main_sizer.is_init())
        init_main_sizer();

    ImVec2 sizer_size = size - GImGui->Style.WindowPadding * 2.f;
    ImVec2 sizer_pos  = GImGui->Style.WindowPadding;

    ImGui::PushStyleColor(ImGuiCol_Button,          ImVec4({ 0.33f, 0.33f, 0.33f, 1.0f }));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,   ImVec4({ 0.923f, 0.504f, 0.264f, 1.0f }));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,    ImVec4({ 0.923f, 0.504f, 0.264f, 1.0f }));

    m_main_sizer.render(sizer_size, sizer_pos);

    ImGui::PopStyleColor(3);
}

}
