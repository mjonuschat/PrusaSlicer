///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"
#include "Slic3r/App/PrintSettingsDialog.hpp"

namespace Slic3r::App {

class PrintSettingsDialog;

namespace Yoga {
class LayoutButton;
class InputTextField;
class ComboBox;
class ScrollArea;
} // namespace Yoga

class SidebarPrint : public Yoga::Window
{
public:
    SidebarPrint();

private:
    void add_separator();
    void add_row(Item* container, const std::string& label, std::unique_ptr<Yoga::Item> control);
    void emplace_tool(const std::string& id);

    void create_favorite_params();
    void create_favorite_params_page(Item* container);

private:
    Yoga::LayoutButton* m_settings_set_btn{nullptr};
    Yoga::ButtonGroup m_group_coordinates;
    Yoga::ButtonGroup m_group_extruder;
    Yoga::InputTextField* m_input_text_perimeters{nullptr};
    Yoga::ComboBox* m_combo_density{nullptr};
    Yoga::ComboBox* m_combo_pattern{nullptr};
    Item* m_tool_container{nullptr};
    std::vector<Item*> m_tools;
    Yoga::ButtonGroup m_group_print_tools;
    Yoga::ScrollArea* m_content_area{nullptr};
    Yoga::ComboBox* m_combo_tools{nullptr};
    Yoga::StackLayout* m_favorite_params_layout{nullptr};

    PrintSettingsDialog m_print_settings_dialog;
};

} // namespace Slic3r::App
