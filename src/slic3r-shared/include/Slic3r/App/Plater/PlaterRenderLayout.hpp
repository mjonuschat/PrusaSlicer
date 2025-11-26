#pragma once

#include "Slic3r/App/AbstractRenderLayout.hpp"

#include "Slic3r/App/Plater/History.hpp"
#include "Slic3r/App/Plater/SidebarPlaterActionButtons.hpp"

namespace Slic3r::App::Plater {

class History;
class SidebarPlaterActionButtons;

class PlaterRenderLayout : public AbstractRenderLayout
{
public:
    PlaterRenderLayout(
        std::unique_ptr<TopBar> top_bar,
        std::unique_ptr<PreferencesDialog> preferences_dialog,
        std::unique_ptr<ObjectListWindow> object_list,
        std::unique_ptr<CubeView> cube_view,
        std::unique_ptr<PopNotification::PopNotificationListView> pop_notification_list_view,
        std::unique_ptr<SidebarBed> sidebar_bed,
        std::unique_ptr<SidebarPrint> sidebar_print,
        std::unique_ptr<SidebarObject> sidebar_object,
        std::unique_ptr<SidebarPlaterActionButtons> sidebar_action_buttons,
        std::unique_ptr<History> history
    );

private:
    void init_left_column() override;
    void init_right_column() override;

private:
    Yoga::Passthrough<SidebarPlaterActionButtons> m_sidebar_action_buttons;
    Yoga::Passthrough<History> m_history;
};

} // namespace Slic3r::App::Plater
