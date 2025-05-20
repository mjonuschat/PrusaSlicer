#include "Slic3r/App/SidebarPrint.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include <imgui/imgui.h>

namespace Slic3r::App {

using namespace Yoga;

SidebarPrint::SidebarPrint() : Window("sidebar_print") {
    set_orientation(Orientation::Vertical);
    set_gap(3);

    m_settings_set_btn = emplace_back<LayoutButton>("Balanced settings", ImGui::SettingsSet, "Tooltip text");
    m_settings_set_btn->align_content(LayoutButton::Align::Left);
    m_settings_set_btn->set_checkable(true);
    m_settings_set_btn->callbacks().action = []() {
        // ToDo open first settings dialog
    };
}

} // namespace Slic3r::App
