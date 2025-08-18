#include "Slic3r/App/Plater/PlaterRenderLayout.hpp"

#include <Yoga.h>

namespace Slic3r::App::Plater {

PlaterRenderLayout::PlaterRenderLayout(
    std::unique_ptr<TopBar> top_bar,
    std::unique_ptr<ObjectListWindow> object_list,
    std::unique_ptr<CubeView> cube_view,
    std::unique_ptr<PopNotification::PopNotificationListView> pop_notification_list_view,
    std::unique_ptr<SidebarBed> sidebar_bed,
    std::unique_ptr<SidebarPrint> sidebar_print,
    std::unique_ptr<SidebarPlaterActionButtons> sidebar_action_buttons,
    std::unique_ptr<History> history
) :
    AbstractRenderLayout(std::move(top_bar), std::move(object_list), std::move(cube_view), std::move(pop_notification_list_view), std::move(sidebar_bed), std::move(sidebar_print)),
    m_sidebar_action_buttons(std::move(sidebar_action_buttons)),
    m_history(std::move(history))
{}

void PlaterRenderLayout::init_left_column()
{
    AbstractRenderLayout::init_left_column();
    m_layout_left_column->append(m_history.release());
}

void PlaterRenderLayout::init_right_column()
{
    AbstractRenderLayout::init_right_column();

    m_layout_right_column->append(m_sidebar_action_buttons.release());
}

} // namespace Slic3r::App::Plater
