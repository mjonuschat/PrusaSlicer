#include "Slic3r/App/SidebarBed.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/PrinterSettingsButton.hpp"
#include "Slic3r/App/Yoga/MaterialSettingsButton.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

#include <imgui/imgui_internal.h>

namespace Slic3r::App {

SidebarBed::SidebarBed() : Window("sidebar_bed")
{
    set_min_size({ 240, 60 });
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

    Item* materials_wrapper = emplace_back<Yoga::Item>();
    materials_wrapper->set_orientation(Yoga::Orientation::Vertical); 
    materials_wrapper->set_gap(5);

    // just for test
    for (auto [color, name, nozzle] : std::initializer_list<std::tuple<ImColor, std::string, float>>{
        {{250, 100, 24 }, "Prusament PLA" , 0.6f},
        {{189, 1,   60 }, "Filamentum PLA", 0.4f},
        {{112, 193, 64 }, "Prusament PETG", 0.4f},
        {{225, 249, 104}, "Filamentum PLA", 0.6f},
    }) {
        Yoga::MaterialSettingsButton* filament = materials_wrapper->emplace_back<Yoga::MaterialSettingsButton>(m_filaments.size()+1, "Filament 1 TT");
        filament->set_color(color);
        filament->set_material_name(name);
        filament->set_nozzle(nozzle);
        filament->callbacks().action = []() {
            // ToDo open first settings dialog
        };

        m_filaments.push_back(filament);
    }
}

} // namespace Slic3r::App
