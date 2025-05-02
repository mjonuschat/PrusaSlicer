#include "Slic3r/App/Plater/PlaterRenderLayout.hpp"

#include "Slic3r/App/Plater/History.hpp"
#include "Slic3r/App/Plater/SidebarPlaterActionButtons.hpp"

#include <Yoga.h>

namespace Slic3r::App::Plater {

PlaterRenderLayout::PlaterRenderLayout(
    ObjectList* object_list,
    CubeView* cube_view,
    SidebarBed* sidebar_bed,
    SidebarPrint* sidebar_print,
    SidebarPlaterActionButtons* sidebar_action_buttons,
    History* history
)
    : AbstractRenderLayout(object_list, cube_view, sidebar_bed, sidebar_print)
    , m_sidebar_action_buttons(sidebar_action_buttons)
    , m_history(history)
{}

void PlaterRenderLayout::init_left_column()
{
    AbstractRenderLayout::init_left_column();
    m_layout_left_column->append(m_history);
}


void PlaterRenderLayout::init_right_column()
{
    AbstractRenderLayout::init_right_column();

    m_layout_right_column->append(m_sidebar_action_buttons);
}

} // namespace Slic3r::App::Plater
