///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

namespace Slic3r::App {

namespace Yoga {
class LayoutButton;
class InputTextField;
class ComboBox;
} // namespace Yoga

class SidebarPrint : public Yoga::Window
{
public:
    SidebarPrint();

private:
    void add_separator();
    void add_row(const std::string& label, std::unique_ptr<Yoga::Item> control);

private:
    Yoga::LayoutButton* m_settings_set_btn{nullptr};
    Yoga::ButtonGroup m_group_coordinates;
    Yoga::ButtonGroup m_group_extruder;
    Yoga::InputTextField* m_input_text_perimeters{nullptr};
    Yoga::ComboBox* m_combo_layer_height{nullptr};
    Yoga::ComboBox* m_combo_density{nullptr};
    Yoga::ComboBox* m_combo_pattern{nullptr};
};

} // namespace Slic3r::App
