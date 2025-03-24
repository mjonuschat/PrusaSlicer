#include "Slic3r/App/Plater/PlaterRenderLayout.hpp"
#include "Slic3r/Log.hpp"

#include <Yoga.h>

namespace Slic3r::App::Plater {

void PlaterRenderLayout::init_left_sizer()
{
    left_sizer.init(1, 2, Vec2f(330.f + win_padding().x(), 0), Yoga::Margins(win_padding() * 0.5f));
    left_sizer.set_bg_alpha(1.f);
    left_sizer.set_grow_col(0);

    add_item(left_sizer, m_cb_object_list_render , "object_list", Yoga::Margins(0.f, frame_padding().y() * 4));
    add_item(left_sizer, m_cb_history_render, "history");

    left_sizer.show_row(1, false);
}

void PlaterRenderLayout::init_right_sizer()
{
    right_sizer.init(1, 3, Vec2f(280.f, 0.f), Yoga::Margins(win_padding() * 0.5f));
    right_sizer.set_bg_alpha(1.f);

    right_sizer.set_grow_col(0);
    for (size_t row_id = 0; row_id < right_sizer.get_rows(); row_id++)
        right_sizer.set_grow_row(row_id, row_id == 1 ? 1.f : 0.f);

    add_item(right_sizer, m_cb_sidebar_bed_render, "bed_settings");
    add_item(right_sizer, m_cb_sidebar_print_render, "print_settings");
    add_item(right_sizer, m_cb_sidebar_slice_render, "slice", Yoga::Margins(20.f, 20.f));
}

}
