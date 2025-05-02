#include "Slic3r/App/SidebarPrint.hpp"

#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include <imgui/imgui.h>

namespace Slic3r::App {

SidebarPrint::SidebarPrint(Item* parent) : Window("sidebar_print", parent) {
    set_orientation(Yoga::Orientation::Vertical);
    set_gap(3);
    set_padding(5);

    new Yoga::LayoutButton("Balanced settings", this);
    new Yoga::LayoutButton("Other button", this);
}

} // namespace Slic3r::App
