#include "Slic3r/App/SidebarPrint.hpp"

#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include <imgui/imgui.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

SidebarPrint::SidebarPrint() : Window("sidebar_print") {
    set_orientation(Yoga::Orientation::Vertical);
    set_gap(3);
    set_padding(5);

    emplace_back<LayoutButton>("Balanced settings");
    emplace_back<LayoutButton>("Other button");
}

} // namespace Slic3r::App
