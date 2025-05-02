#pragma once

#include "Slic3r/App/AbstractRenderLayout.hpp"

namespace Slic3r::App {
class SidebarActionButtons;
}

namespace Slic3r::App::Plater {

class History;
class SidebarPlaterActionButtons;

class PlaterRenderLayout : public AbstractRenderLayout
{
public:
    PlaterRenderLayout(
        ObjectList* object_list,
        CubeView* cube_view,
        SidebarBed* sidebar_bed,
        SidebarPrint* sidebar_print,
        SidebarPlaterActionButtons* sidebar_action_buttons,
        History* history
    );

private:
    void init_left_column() override;
    void init_right_column() override;

private:
    SidebarPlaterActionButtons* m_sidebar_action_buttons = nullptr;
    History* m_history = nullptr;
};

} // namespace Slic3r::App::Plater
