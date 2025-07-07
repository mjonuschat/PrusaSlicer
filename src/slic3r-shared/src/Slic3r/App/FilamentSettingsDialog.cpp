///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/FilamentSettingsDialog.hpp"

#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace Slic3r::App {

FilamentSettingsDialog::FilamentSettingsDialog()
    : AbstractSettingsDialog({"Filament 1", "Filament 2", "Filament 3", "Filament 4"})
{
    content_item()->set_width(650);
    content_item()->set_height(700);

    m_list_pages.reset(
        {{_u8L("Filament")},
         {_u8L("Cooling")},
         {_u8L("Advanced")},
         {_u8L("Filament overrides")},
         {_u8L("Custom G-code")},
         {_u8L("Notes")},
         {_u8L("Dependencies")}}
    );
    m_page_list_view->set_source_list(&m_list_pages);

    dynamic_cast<PageEntryButton*>(m_page_list_view->get_item(0))->set_checked(true);

    // This is stub, needs to be implemented dynamically
    ScrollArea* filament = emplace_stack_page();

    {
        std::vector<RowItem> fields;
        fields.emplace_back(std::make_unique<InputTextField>(), "Diameter", "mm");
        fields.emplace_back(std::make_unique<InputTextField>(), "Extrusion multiplier", "");
        fields.emplace_back(std::make_unique<InputTextField>(), "Density", "g/cm3");
        fields.emplace_back(std::make_unique<InputTextField>(), "Cost", "money/kg");
        fields.emplace_back(std::make_unique<InputTextField>(), "Spool weight", "g");
        emplace_subcategory(filament, "Filament", "", std::move(fields));
    }

    {
        std::vector<RowItem> fields;
        fields.emplace_back(std::make_unique<InputTextField>(), "Idle temperature", "°C");
        fields.emplace_back(std::make_unique<InputTextField>(), "Nozzle first layer", "°C");
        fields.emplace_back(std::make_unique<InputTextField>(), "Nozzle other layers", "°C");
        fields.emplace_back(std::make_unique<InputTextField>(), "Bed first layer", "°C");
        fields.emplace_back(std::make_unique<InputTextField>(), "Bed other layers", "°C");
        fields.emplace_back(std::make_unique<InputTextField>(), "Chamber nominal", "°C");
        fields.emplace_back(std::make_unique<InputTextField>(), "Chamber minimal", "°C");
        emplace_subcategory(filament, "Temperatures", "", std::move(fields));
    }


    emplace_stack_page(); // Cooling
    emplace_stack_page(); // Advanced
    emplace_stack_page(); // Filament overrides
    emplace_stack_page(); // Custom G-code
    emplace_stack_page(); // Notes
    emplace_stack_page(); // Dependencies
}

} // namespace Slic3r::App
