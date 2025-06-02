#include "Slic3r/App/SidebarBed.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"
#include "Slic3r/App/Yoga/MaterialSettingsButton.hpp"

#include <imgui/imgui_internal.h>

namespace Slic3r::App {

SidebarBed::SidebarBed() : Window("sidebar_bed")
{
    set_min_size({240, 60});
    set_orientation(Yoga::Orientation::Vertical);
    set_gap(10);

    m_bed_name = emplace_back<Yoga::Text>("Bed 1");
    m_bed_name->set_font_type(Render::ImguiFontType::Bold);

    m_printer = emplace_back<Yoga::PrinterSettingsButton>("Printer Tooltip");
    m_printer->set_icon(Render::Icon::PrinterNEXT);
    m_printer->set_printer_name("NEXT/Elsa");
    m_printer->set_preset_name("Prusa NEXT 1T");
    m_printer->set_visible_cog(true);

    m_printer->callbacks().action = []() {
        // ToDo open first settings dialog
    };

    m_printer->on_cog() = []() {
        // ToDo open some other settings dialog
    };

    m_observable_list.reset(
        {{ImColor{250, 100, 24}, "Prusament PLA", 0.6f},
         {ImColor{189, 1, 60}, "Filamentum PLA", 0.4f},
         {ImColor{112, 193, 64}, "Prusament PETG", 0.4f},
         {ImColor{225, 249, 104}, "Filamentum PLA", 0.6f}}
    );
    m_list_view = emplace_back<MaterialListView>();
    m_list_view->set_source_list(&m_observable_list);
    m_list_view->set_orientation(Yoga::Orientation::Vertical);
    m_list_view->set_gap(5);
}

} // namespace Slic3r::App
