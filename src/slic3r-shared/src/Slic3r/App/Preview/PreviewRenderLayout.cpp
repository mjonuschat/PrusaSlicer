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

void PreviewRenderLayout::add_middle_flex_sizer()
{
    // flex sizer on the right side of toolbar

    middle_left_flex_sizer.init(1, 2);
    middle_left_flex_sizer.set_grow_col(0);
    middle_left_flex_sizer.set_grow_row(1, 0.f);

    static Yoga::FlexSizer gcode_sizer(1, 1, ImVec2(0.f, 50.f));
    gcode_sizer.set_bg_alpha(1.f);
    gcode_sizer.set_grow_col(0);
    gcode_sizer.add(m_cb_gcode_slider_render, { Yoga::AlignH::Left, Yoga::AlignV::Top }, "gcode_slider");

    middle_left_flex_sizer.add(view_cube_sizer, "", { Yoga::AlignH::Right, Yoga::AlignV::Top });
    middle_left_flex_sizer.add(gcode_sizer);

    middle_flex_sizer.init(2, 1);
    middle_flex_sizer.set_grow_col(0);

    static Yoga::FlexSizer slider_sizer(1, 1, ImVec2(80.f, 0.f));
    slider_sizer.set_bg_alpha(1.f);
    slider_sizer.set_grow_col(0);
    slider_sizer.add(m_cb_layer_slider_render, { Yoga::AlignH::Left, Yoga::AlignV::Top }, "layers_slider");

    middle_flex_sizer.add(middle_left_flex_sizer);
    middle_flex_sizer.add(slider_sizer);

    middle_sizer.add(middle_flex_sizer);
}

void PreviewRenderLayout::init_right_sizer()
{
    static Yoga::FlexSizer sidebar_sizer(1, 4, ImVec2(280.f, 0.f), GImGui->Style.WindowPadding * 0.5f);
    sidebar_sizer.set_bg_alpha(1.f);
    sidebar_sizer.set_grow_col(0);
    for (size_t row_id = 0; row_id < sidebar_sizer.get_rows(); row_id++)
        sidebar_sizer.set_grow_row(row_id, row_id == 1 ? 1.f : 0.f);

    add_item(sidebar_sizer, m_cb_sidebar_bed_render, "bed_settings");
    add_item(sidebar_sizer, m_cb_sidebar_print_render, "print_settings");
    add_item(sidebar_sizer, m_cb_sidebar_auto_reslice_render, "auto re-slice", ImVec2(20.f, 20.f));
    add_item(sidebar_sizer, m_cb_sidebar_after_slice_render, "after slice", ImVec2(20.f, 20.f));

    right_sizer.init(1, 1);
    right_sizer.set_grow_col(0);
    right_sizer.add(sidebar_sizer);
}

void PreviewRenderLayout::show_gcode_sizer(bool show)
{
    middle_left_flex_sizer.show_row(1, show);
}

void PreviewRenderLayout::show_slider_sizer(bool show)
{
    middle_flex_sizer.show_col(1, show);
}

}
