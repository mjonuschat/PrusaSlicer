#include "Slic3r/App/Preview/PreviewRenderLayout.hpp"

#include "Slic3r/App/Preview/GCodeWindow.hpp"
#include "Slic3r/App/Preview/Legend.hpp"
#include "Slic3r/App/Preview/DoubleSliderForLayers.hpp"
#include "Slic3r/App/Preview/DoubleSliderForGCode.hpp"
#include "Slic3r/App/Preview/SidebarAutoReslice.hpp"
#include "Slic3r/App/Preview/SidebarPreviewActionButtons.hpp"

namespace Slic3r::App::Preview {

PreviewRenderLayout::PreviewRenderLayout(
    ObjectList* object_list,
    CubeView* cube_view,
    SidebarBed* sidebar_bed,
    SidebarPrint* sidebar_print,
    SidebarPreviewActionButtons* sidebar_action_buttons,
    GCodeWindow* m_gcode_window,
    Legend* legend,
    DoubleSliderForLayers* double_slider_layers,
    DoubleSliderForGcode* double_slider_gcode,
    SidebarAutoReslice* sidebar_auto_reslice
)
    : AbstractRenderLayout(object_list, cube_view, sidebar_bed, sidebar_print)
    , m_gcode_window(m_gcode_window)
    , m_legend(legend)
    , m_double_slider_layers(double_slider_layers)
    , m_double_slider_gcode(double_slider_gcode)
    , m_sidebar_auto_reslice(sidebar_auto_reslice)
    , m_sidebar_action_buttons(sidebar_action_buttons)
{}

void PreviewRenderLayout::init_left_column()
{
    AbstractRenderLayout::init_left_column();

    m_layout_left_column->append(m_legend);
    m_legend->set_visible(false);
    m_legend->set_min_size({0, 100});

    m_layout_left_column->append(m_gcode_window);
    m_gcode_window->set_visible(false);
    m_gcode_window->set_min_size({0, 100});
}

void PreviewRenderLayout::init_middle_column()
{
    AbstractRenderLayout::init_middle_column();

    // flexbox doesnt support justify-self, spacer is needed
    Yoga::Item* column_spacer = new Yoga::Item(m_layout_middle_column);
    column_spacer->set_flex_grow(1);
    m_layout_middle_column->append(m_double_slider_gcode);

    m_layout_center_row->append(m_double_slider_layers);
}

void PreviewRenderLayout::init_right_column()
{
    AbstractRenderLayout::init_right_column();

    m_sidebar_auto_reslice = new SidebarAutoReslice;
    m_layout_right_column->append(m_sidebar_auto_reslice);

    m_layout_right_column->append(m_sidebar_action_buttons);
}

} // namespace Slic3r::App::Preview
