#pragma once

#include "Slic3r/App/SidebarActionButtons.hpp"

namespace Slic3r::App::Yoga {
class LayoutButton;
}

namespace Slic3r::App::Plater {

class SidebarPlaterActionButtons : public SidebarActionButtons
{
public:
    explicit SidebarPlaterActionButtons(Yoga::Item* parent = nullptr);

private:
    Yoga::LayoutButton* m_button_slice = nullptr;
};

} // namespace Slic3r::App::Plater
