#include "Slic3r/App/Preview/PreviewRenderLayout.hpp"
#include "Slic3r/Log.hpp"

#include <Yoga.h>

#include <imgui/imgui_internal.h>

namespace Slic3r::App::Preview {

void PreviewRenderLayout::init_left_sizer()
{
    left_sizer.init(1, 2, ImVec2(330.f + GImGui->Style.WindowPadding.x, 0), GImGui->Style.WindowPadding * 0.5f);
    left_sizer.set_bg_alpha(1.f);
    left_sizer.set_grow_col(0);

    add_item(left_sizer, m_cb_object_list_render , "object_list", ImVec2(0.f, GImGui->Style.FramePadding.y * 4));
    add_item(left_sizer, m_cb_legend_render, "legend");

    left_sizer.show_row(1, false);
}

void PreviewRenderLayout::init_middle_sizer()
{
    middle_sizer.initialize();

    // add items with callbacks for top corner toolbars

    // callbacks for toolbar items
    auto cb_is_visible      = []() -> bool {return true; };
    auto cb_is_enable       = []() -> bool {return true; };

    static bool show_object_list   { true };
    static bool show_legend   { false };
    static bool show_sidebar  { true };

    // create top left toolbar, which contains just one item
    middle_sizer.top_left_toolbar.add(ImGui::ToolbarObjects, "Object List", "Ctrl + Alt + O",  
        { [this](ImRect) { 
            show_object_list = !show_object_list; 
            show_left(show_object_list); 
          }, 
          cb_is_visible, cb_is_enable, []() { return !show_object_list; } });

    // create top right toolbar, which contains just one item
    middle_sizer.top_right_toolbar.add(ImGui::ToolbarSidebar, "Sidebar", "", 
        { [this](ImRect) { 
            show_sidebar = !show_sidebar;
            show_right(show_sidebar); 
          }, 
          cb_is_visible, cb_is_enable, []() { return !show_sidebar; } });

//    middle_sizer.left_middle_toolbar.add(ImGui::ToolbarAdd, "Add...", "Ctrl + I", { [](ImRect) {}, cb_is_visible, cb_is_enable, []() { return true; } });

    // create bottom left toolbar

    middle_sizer.bottom_left_toolbar.add(ImGui::ToolbarGraph, "Legend", "",
        { [this](ImRect) { 
                show_legend = !show_legend;
                left_sizer.show_row(1, show_legend);
            }, cb_is_visible, cb_is_enable, []() { return show_legend; } });

    middle_sizer.layout();
}

void PreviewRenderLayout::init_right_sizer()
{
    static Yoga::FlexSizer slider_sizer(1, 1, ImVec2(80.f, 0.f), GImGui->Style.WindowPadding * 0.5f);
    slider_sizer.set_bg_alpha(1.f);
    slider_sizer.set_grow_col(0);
    add_item(slider_sizer, m_cb_layer_slider_render, "layers_slider");

    static Yoga::FlexSizer sidebar_sizer(1, 4, ImVec2(280.f, 0.f), GImGui->Style.WindowPadding * 0.5f);
    sidebar_sizer.set_bg_alpha(1.f);
    sidebar_sizer.set_grow_col(0);
    for (size_t row_id = 0; row_id < sidebar_sizer.get_rows(); row_id++)
        sidebar_sizer.set_grow_row(row_id, row_id == 1 ? 1.f : 0.f);

    add_item(sidebar_sizer, m_cb_sidebar_bed_render, "bed_settings");
    add_item(sidebar_sizer, m_cb_sidebar_print_render, "print_settings");
    add_item(sidebar_sizer, m_cb_sidebar_auto_reslice_render, "auto re-slice", ImVec2(20.f, 20.f));
    add_item(sidebar_sizer, m_cb_sidebar_after_slice_render, "after slice", ImVec2(20.f, 20.f));

    right_sizer.init(2, 1);

    right_sizer.add(slider_sizer);
    right_sizer.add(sidebar_sizer);
}

void PreviewRenderLayout::show_bottom_middle_sizer(bool show)
{
    middle_sizer.bottom_middle_sizer.show_col(0, show);
}

}
