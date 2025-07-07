///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrinterAdvancedSettingsDialog.hpp"

#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

PrinterAdvancedSettingsDialog::PrinterAdvancedSettingsDialog() : AbstractSettingsDialog("Printer")
{
    content_item()->set_width(650);
    content_item()->set_height(700);

    m_list_pages.reset(
        {{_u8L("General")},
         {_u8L("Custom G-code")},
         {_u8L("Machine limits")},
         {_u8L("Extruder 1")},
         {_u8L("Extruder 2")},
         {_u8L("Extruder 3")},
         {_u8L("Extruder 4")},
         {_u8L("Notes")},
         {_u8L("Dependencies")}}
    );
    m_page_list_view->set_source_list(&m_list_pages);

    dynamic_cast<PageEntryButton*>(m_page_list_view->get_item(0))->set_checked(true);

    // This is stub, needs to be implemented dynamically
    ScrollArea* general = emplace_stack_page();

    {
        std::vector<RowItem> fields;
        fields.emplace_back(std::make_unique<LayoutButton>("Set bed shape"), "Bed shape", "");
        fields.emplace_back(std::make_unique<InputTextField>(), "Max print height", "mm");
        fields.emplace_back(std::make_unique<InputTextField>(), "Z offset", "mm");
        emplace_subcategory(general, "Size and coordinates", "", std::move(fields));
    }

    {
        std::vector<RowItem> fields;
        fields.emplace_back(std::make_unique<InputTextField>(), "Extruders", "");
        fields.emplace_back(std::make_unique<ToggleButton>(), "Single extruder Multi Material", "");
        emplace_subcategory(general, "Capabilities", "", std::move(fields));
    }

    {
        std::vector<RowItem> fields;
        fields.emplace_back(
            std::make_unique<ComboBox>(std::initializer_list<std::string>{"Marlin 2"}),
            "G-code flavour", ""
        );
        fields.emplace_back(std::make_unique<InputTextField>(), "G-code thumbnails", "");
        fields.emplace_back(std::make_unique<ToggleButton>(), "Supports stealth mode", "");
        fields.emplace_back(std::make_unique<ToggleButton>(), "Supports remaining times", "");
        fields.emplace_back(std::make_unique<ToggleButton>(), "Supports binary G-code", "");
        emplace_subcategory(general, "Firmware", "", std::move(fields));
    }

    {
        std::vector<RowItem> fields;
        fields.emplace_back(
            std::make_unique<ComboBox>(std::initializer_list<std::string>{"Aligned"}),
            "Seam position", ""
        );
        fields.emplace_back(std::make_unique<ToggleButton>(), "Staggered inner seams", "");
        fields.emplace_back(std::make_unique<ToggleButton>(), "Fill gaps", "");
        fields.emplace_back(
            std::make_unique<ComboBox>(std::initializer_list<std::string>{"Arachne"}),
            "Parameter generator", ""
        );
        emplace_subcategory(general, "Advanced", "", std::move(fields));
    }

    emplace_stack_page(); // Custom G-code
    emplace_stack_page(); // Machine limits
    emplace_stack_page(); // Extruder 1
    emplace_stack_page(); // Extruder 2
    emplace_stack_page(); // Extruder 3
    emplace_stack_page(); // Extruder 4
    emplace_stack_page(); // Notes
    emplace_stack_page(); // Dependencies
}

} // namespace Slic3r::App
