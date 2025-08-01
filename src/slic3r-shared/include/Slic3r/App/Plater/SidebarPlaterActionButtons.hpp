#pragma once

#include "Slic3r/App/SidebarActionButtons.hpp"

namespace Slic3r::App::Yoga {
class LayoutButton;
}

namespace Slic3r::App::Plater {

class SidebarPlaterActionButtons : public SidebarActionButtons
{
public:
    SidebarPlaterActionButtons(Navigator* render_module_navigator);

private:
    Yoga::LayoutButton* m_button_slice = nullptr;
};

} // namespace Slic3r::App::Plater
