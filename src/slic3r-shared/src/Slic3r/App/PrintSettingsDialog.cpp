///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/PrintSettingsDialog.hpp"

#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

using namespace Slic3r::App::Yoga;
using namespace Slic3r::App::Render;

namespace Slic3r::App {

PrintSettingsDialog::PrintSettingsDialog()
    : AbstractSettingsDialog({"Print settings", "Tool 1", "Tool 2", "Tool 3", "Tool 4"})
{
    content_item()->set_width(650);
    content_item()->set_height(700);

    // Keep in mind all content fill right now is purely for demo only

    m_list_pages_print.reset({
        {_u8L("Layers and perimeters"), Render::Icon::Layers},
        {_u8L("Infill"), Render::Icon::Infill},
        {_u8L("Skirt & Brim"), Render::Icon::SkirtBrim},
        {_u8L("Supports"), Render::Icon::Support},
        {_u8L("Speed"), Render::Icon::Time},
        {_u8L("Extruders"), Render::Icon::Funnel},
        {_u8L("Advanced"), Render::Icon::Cogs},
        {_u8L("Output"), Render::Icon::Output},
        {_u8L("Notes"), Render::Icon::Notes},
        {_u8L("Dependencies"), Render::Icon::Cogs},
    });
    m_list_pages_tool.reset(
        {{_u8L("Layers and perimeters"), Render::Icon::Layers},
         {_u8L("Infill"), Render::Icon::Infill},
         {_u8L("Skirt & Brim"), Render::Icon::SkirtBrim},
         {_u8L("Supports"), Render::Icon::Support},
         {_u8L("Speed"), Render::Icon::Time},
         {_u8L("Extruders"), Render::Icon::Funnel}}
    );
    m_page_list_view->set_source_list(&m_list_pages_print);
    dynamic_cast<PageEntryButton*>(m_page_list_view->get_item(0))->set_checked(true);

    // This is stub, needs to be implemented dynamically
    ScrollArea* layers_and_perimeters = emplace_stack_page();

    {
        std::vector<RowItem> fields;
        fields.emplace_back(std::make_unique<InputTextField>(), "Layer height", "mm");
        fields.emplace_back(std::make_unique<InputTextField>(), "First layer height", "mm");
        emplace_subcategory(layers_and_perimeters, "Layer height", "", std::move(fields));
    }

    {
        std::vector<RowItem> fields;
        fields.emplace_back(
            std::make_unique<ComboBox>(std::initializer_list<std::string>{"1", "2", "3"}),
            "Perimeters", "(minimum)"
        );
        fields.emplace_back(
            std::make_unique<ComboBox>(std::initializer_list<std::string>{"Off", "On"}),
            "Spiral vase"
        );
        emplace_subcategory(
            layers_and_perimeters, "Vertical shells",
            "Recommended object thin wall thickness for layer height 0.15 and 2 lines: 0.87 "
            "mm, 4 "
            "lines: 1.70 mm",
            std::move(fields)
        );
    }

    {
        std::vector<RowItem> fields;
        fields.emplace_back(std::make_unique<InputTextField>(), "Solid layers top");
        fields.emplace_back(std::make_unique<InputTextField>(), "Solid layers bottom");
        fields.emplace_back(std::make_unique<InputTextField>(), "Minimum shell thickness top", "mm");
        fields
            .emplace_back(std::make_unique<InputTextField>(), "Minimum shell thickness bottom", "mm");
        emplace_subcategory(
            layers_and_perimeters, "Horizontal shells",
            "Top shell is 1.2 mm thick for layer height 0.15 mm. Minimum top shell thickness "
            "is 0.7 "
            "mm. Bottom shell is 0.75 mm thick for layer height 0.15 mm. Minimum bottom shell "
            "thickness is 0.5 mm.",
            std::move(fields)
        );
    }

    {
        std::vector<RowItem> fields;
        fields.emplace_back(std::make_unique<ToggleButton>(), "Ensure vertical shell thickness");
        fields.emplace_back(std::make_unique<ToggleButton>(), "Detect thin walls");
        fields.emplace_back(std::make_unique<ToggleButton>(), "Thick bridges");
        fields.emplace_back(std::make_unique<ToggleButton>(), "Detect bridging parameters");
        emplace_subcategory(layers_and_perimeters, "Quality (slower slicing)", "", std::move(fields));
    }

    {
        std::vector<RowItem> fields;
        fields.emplace_back(
            std::make_unique<ComboBox>(std::initializer_list<std::string>{"Aligned"}),
            "Seam position"
        );
        fields.emplace_back(std::make_unique<ToggleButton>(), "Staggered inner seams");
        fields.emplace_back(std::make_unique<ToggleButton>(), "Fill gaps");
        fields.emplace_back(
            std::make_unique<ComboBox>(std::initializer_list<std::string>{"Arachne"}),
            "Parameter generator"
        );
        emplace_subcategory(layers_and_perimeters, "Advanced", "", std::move(fields));
    }

    emplace_stack_page(); // infill
    emplace_stack_page(); // skirt and brim
    emplace_stack_page(); // supports
    emplace_stack_page(); // speed
    emplace_stack_page(); // extruders
    emplace_stack_page(); // advanced
    emplace_stack_page(); // output
    emplace_stack_page(); // notes
    emplace_stack_page(); // dependencies

    m_footer->emplace_back<LayoutButton>("Save preset");
}

void PrintSettingsDialog::on_tab_selected(int current_index) {}

} // namespace Slic3r::App
