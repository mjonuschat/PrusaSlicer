#pragma once
#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App {

class SidebarPrint : public Yoga::Window
{
public:
    explicit SidebarPrint(Yoga::Item* parent = nullptr);
};

} // namespace Slic3r::App
