#include "Slic3r/App/Yoga/MiddleSizer.hpp"
#include "Slic3r/Log.hpp"

#include <Yoga.h>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r::App::Yoga {

static bool     show_tmp_window = false;
static ImVec2   tmp_window_pos = ImVec2();

static void tmp_window()
{
    if (!show_tmp_window)
        return;

    ImGui::SetNextWindowPos(tmp_window_pos);
    ImGui::SetNextWindowSize(ImVec2(-1.f, 100.f));

    ImGui::Begin("Some arrow is clicked", &show_tmp_window, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("This is callback for Arrow pressing");
    ImGui::End();
}

const static float min_tt_size = 30.f;
const static float max_tt_size = 60.f;

void MiddleSizer::initialize()
{
    left_middle_toolbar.init("L", min_tt_size, max_tt_size, { AlignH::Left, AlignV::Center }, FlexToolbarOrientation::Vertical);
    left_middle_toolbar.collapse_if_needed();

    init_middle_top_sizer();
    init_middle_bottom_sizer();

    this->init(1, 3, ImVec2(350.f, 350.f));
    this->set_grow_col(0);
    this->set_bg_alpha(0.15f);

    this->set_grow_row(0, float(br_toolbar.shown_items_cnt()));
    this->set_grow_row(2, float(br_toolbar.shown_items_cnt()));

    static FlexSizer middle_sizer(1, 1, ImVec2(), ImVec2(5.f, 5.f));
    middle_sizer.set_grow_col(0);

    middle_sizer.add([this](ImVec2 size, ImVec2 win_pos) {
        left_middle_toolbar.render(size, win_pos);
    }, Align{ AlignH::Left, AlignV::Top });

    this->add(m_top_sizer);
    this->add(middle_sizer);
    this->add(m_bottom_sizer);
}

void MiddleSizer::init_middle_top_sizer()
{
    top_left_toolbar.init("s_h_top_left", min_tt_size, max_tt_size, { AlignH::Left, AlignV::Top });

    top_middle_toolbar.init("T", min_tt_size, max_tt_size, { AlignH::Center, AlignV::Top }, FlexToolbarOrientation::Horizontal);
    top_middle_toolbar.collapse_if_needed();

    top_right_toolbar.init("s_h_top_right", min_tt_size, max_tt_size, { AlignH::Right, AlignV::Top });

    // initialize top sizer and add toolbars

    m_top_sizer.init(3, 1, ImVec2(), ImVec2(5.f, 0.f));
    m_top_sizer.set_grow_col(0, 1.f);
    m_top_sizer.set_grow_col(1, 1.f);
    m_top_sizer.set_grow_col(2, 1.f);

    m_top_sizer.add([this](ImVec2 size, ImVec2 pos) {
        top_left_toolbar.render(size, pos);
    });

    m_top_sizer.add([this](ImVec2 size, ImVec2 win_pos) {
        top_middle_toolbar.render(size, win_pos);
    });

    m_top_sizer.add([this](ImVec2 size, ImVec2 pos) {
        top_right_toolbar.render(size, pos);
    });
}

void MiddleSizer::init_middle_bottom_left_toolbar()
{
    // Create sub tooltips

    static FlexToolbar sub_left_toolbar("sub_left", min_tt_size, max_tt_size, { AlignH::Left, AlignV::Top }, FlexToolbarOrientation::Vertical);
    sub_left_toolbar.add("sub1", "sub 1 tooltip", {[](ImRect) { SPDLOG_INFO("sub 1 left is pressed"); }});
    sub_left_toolbar.add_separator(1.f);
    sub_left_toolbar.add("sub2", "sub 2 tooltip", {[](ImRect) { SPDLOG_INFO("sub 2 left is pressed"); }});
    sub_left_toolbar.add("sub3", "sub 3 tooltip", {[](ImRect) { SPDLOG_INFO("sub 3 left is pressed"); }});

    static FlexToolbar sub_top_toolbar("sub_left", min_tt_size, max_tt_size, { AlignH::Left, AlignV::Top }, FlexToolbarOrientation::Horizontal);
    sub_top_toolbar.add("sub1t", "sub 1 tooltip", {[](ImRect) { SPDLOG_INFO("sub 1 top is pressed"); }});
    sub_top_toolbar.add("sub2t", "sub 2 tooltip", {[](ImRect) { SPDLOG_INFO("sub 2 top is pressed"); }});
    sub_top_toolbar.add_separator(1.f);
    sub_top_toolbar.add("sub3t", "sub 3 tooltip", {[](ImRect) { SPDLOG_INFO("sub 3 top is pressed"); }});

    // initialize left toolbar for bottom sizer and add toolbar items

    bl_toolbar.init("add_del", min_tt_size, max_tt_size, { AlignH::Left, AlignV::Bottom }, FlexToolbarOrientation::Horizontal);

    bl_toolbar.add("+ I", "Add item for L&T", { [this](ImRect) {
        if (top_middle_toolbar.shown_items_cnt() == 1)
            top_middle_toolbar.add("t...", &sub_top_toolbar);
        else
            top_middle_toolbar.add("", "", {[](ImRect) { SPDLOG_INFO("top_tb btn is pressed"); }});

        if (left_middle_toolbar.shown_items_cnt() == 2)
            left_middle_toolbar.add("l...", &sub_left_toolbar);
        else
            left_middle_toolbar.add("", "", {[](ImRect) { SPDLOG_INFO("left_tb btn is pressed"); }});

        layout();
    } });

    bl_toolbar.add("+ S", "Add separator for L&T", { [this](ImRect) {
        top_middle_toolbar.add_separator(3.f);
        left_middle_toolbar.add_separator(4.f);
    } });

    bl_toolbar.add_separator(5.f);

    bl_toolbar.add("+ AI L", "Add ArrowItem for Left", { [this](ImRect) {
        FlexToolbarItem& item = left_middle_toolbar.add("", "", {[](ImRect) { SPDLOG_INFO("left_tb btn is pressed"); }});
        item.set_action_on_arrow([](ImRect) {
            SPDLOG_INFO("Arrow clicked");
        });

        std::string win_name = "Arrow hovered##left_arrow_win" + std::to_string(left_middle_toolbar.shown_items_cnt());
        item.set_action_on_arrow_hovering ( [win_name](ImRect bb) {
            ImGui::SetNextWindowPos(bb.Max);
            ImGui::SetNextWindowSize(ImVec2(-1.f, 100.f));

            ImGui::Begin(win_name.c_str(), nullptr, ImGuiWindowFlags_NoMove);
            ImGui::Text("This is callback for Arrow hovering");
            ImGui::End();
        });

        layout();
    } });

    bl_toolbar.add_separator(5.f);

    bl_toolbar.add("-Left", "Delete item from left", { [this](ImRect) {
        left_middle_toolbar.erase();
        layout();
    } });
}

void MiddleSizer::init_middle_bottom_right_toolbar()
{
    br_toolbar.init("del", min_tt_size, max_tt_size, { AlignH::Right, AlignV::Bottom });

    br_toolbar.add("+ AI T", "Add ArrowItem for Top", { [this](ImRect) {
        FlexToolbarItem& item = top_middle_toolbar.add("", "", {[](ImRect) { SPDLOG_INFO("left_tb btn is pressed"); }});
        item.set_action_on_arrow([](ImRect bb) {
            show_tmp_window = true; 
            tmp_window_pos = bb.GetCenter();
        });

        layout();
    } });

    br_toolbar.add_separator(5.f);

    br_toolbar.add("-Top", "Delete item from top", { [this](ImRect) {
        top_middle_toolbar.erase();
        layout();
    } });
}

void MiddleSizer::init_middle_bottom_sizer()
{
    init_middle_bottom_left_toolbar();
    init_middle_bottom_right_toolbar();

    m_bottom_sizer.init(3, 1, ImVec2(), ImVec2(5.f, 0.f));
    m_bottom_sizer.set_grow_col(0, float(bl_toolbar.shown_items_cnt()));
    m_bottom_sizer.set_grow_col(1, 0.f);
    m_bottom_sizer.set_grow_col(2, 1.f);

    m_bottom_sizer.add([this](ImVec2 size, ImVec2 pos) {
        bl_toolbar.render(size, pos);
    });

    m_bottom_sizer.add();

    m_bottom_sizer.add([this](ImVec2 size, ImVec2 pos) {
        br_toolbar.render(size, pos);
    });
}

void MiddleSizer::layout()
{
    int mid_top_items = top_middle_toolbar.shown_items_cnt();

    m_top_sizer.set_grow_col(1, float(mid_top_items));
    m_bottom_sizer.set_grow_col(1, std::max(0.f, float(mid_top_items - bl_toolbar.shown_items_cnt() + 1)));
    this->set_grow_row(1, float(left_middle_toolbar.shown_items_cnt()));

    FlexSizer::layout();
    m_top_sizer.layout();
    m_bottom_sizer.layout();
}

void MiddleSizer::render(ImVec2 win_size, ImVec2 win_pos)
{
    FlexSizer::render(win_size, win_pos);

    tmp_window();
}

}
