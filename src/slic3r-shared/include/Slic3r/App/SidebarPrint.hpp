#pragma once
#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App {

namespace Yoga {
    class LayoutButton;
}

class SidebarPrint : public Yoga::Window
{
public:
    SidebarPrint();

private:
    Yoga::LayoutButton* m_settings_set_btn{ nullptr };
};

} // namespace Slic3r::App
