#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App::Yoga {
    class ToggleButton;
}

namespace Slic3r::App::Preview {

class SidebarAutoReslice : public Yoga::Window
{
public:
    SidebarAutoReslice();

public:
    Yoga::ToggleButton* m_auto_reslice_chb { nullptr };
};

} // namespace Slic3r::App::Preview
