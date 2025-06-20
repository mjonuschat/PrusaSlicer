#include "Slic3r/App/SidebarPrint.hpp"

#include "Slic3r/App/Yoga/RectangleButton.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Separator.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/RadioButton.hpp"
#include "Slic3r/App/Yoga/RadioExtruder.hpp"

#include "Slic3r/App/I18N/I18N.hpp"

#include <Slic3r/Log.hpp>

#include <imgui/imgui.h>

namespace Slic3r::App {

using namespace Yoga;

SidebarPrint::SidebarPrint() : Window("sidebar_print")
{
    set_orientation(Orientation::Vertical);
    set_gap(5);
    set_flex_grow(1);

    m_settings_set_btn =
        emplace_back<LayoutButton>("Balanced settings", Render::Icon::SettingsSet, "Tooltip text");
    m_settings_set_btn->align_content(RectangleButton::Align::Left);
    m_settings_set_btn->set_checkable(true);
    m_settings_set_btn->set_background_color(ImColor(41, 41, 41));
    m_settings_set_btn->callbacks().action = []() {
        // ToDo open first settings dialog
    };

    const Vec2f button_size{ 24.f, 24.f };
    constexpr float gap_size = 10;

    std::unique_ptr<Item> coordinates = std::make_unique<Item>();
    coordinates->set_gap(gap_size);

    for (const std::string& name : { _u8L("World"), _u8L("Object"), _u8L("Part"), }) {
        RadioButton* radio_btn =
            coordinates->emplace_back<RadioButton>(name);
        radio_btn->set_checkable(true);
        if (name == _u8L("World"))
            radio_btn->set_checked(true);
        m_group_coordinates.insert_button(radio_btn);
    }
    add_row("Coordinates", std::move(coordinates));

    add_separator();

    std::unique_ptr<Item> extruders_selector = std::make_unique<Item>();
    extruders_selector->set_gap(gap_size);

    size_t extruer_id = 0;
    for (const ImColor& color : std::initializer_list<ImColor> { 
                                ImColor{250, 100, 24},
                                ImColor{189, 1, 60},
                                ImColor{112, 193, 64},
                                ImColor{225, 249, 104} }) {
        RadioExtruder* radio_btn = extruders_selector->emplace_back<RadioExtruder>(++extruer_id, color);
        radio_btn->set_checkable(true);
        if (extruer_id == 1)
            radio_btn->set_checked(true);
        radio_btn->set_border_width(2);
        radio_btn->set_min_size(button_size);
        m_group_extruder.insert_button(radio_btn);
    }
    add_row("Extruders", std::move(extruders_selector));

    add_separator();

    std::unique_ptr<ComboBox> combo_layer_height = std::make_unique<ComboBox>(
        std::initializer_list<std::string>{{"0.2 mm"}, {"0.3 mm"}, {"0.4 mm"}}
    );
    m_combo_layer_height = combo_layer_height.get();
    m_combo_layer_height->set_editable(true);
    m_combo_layer_height->set_validator(std::make_unique<DoubleValidator>());
    add_row("Layer Height", std::move(combo_layer_height));

    add_separator();

    std::unique_ptr<InputTextField> input_text = std::make_unique<InputTextField>();
    m_input_text_perimeters = input_text.get();
    m_input_text_perimeters->set_validator(std::make_unique<IntValidator>());
    m_input_text_perimeters->set_flags(ImGuiInputTextFlags_CharsDecimal);
    add_row("Perimeters", std::move(input_text));

    add_separator();

    add_row("Infill", std::make_unique<Item>());

    std::unique_ptr<ComboBox> combo_density = std::make_unique<ComboBox>(
        std::initializer_list<std::string>{{"15%"}}
    );
    m_combo_density = combo_density.get();
    add_row("Density", std::move(combo_density));

    std::unique_ptr<ComboBox> combo_pattern = std::make_unique<ComboBox>(
        std::initializer_list<std::string>{{"Grid"}}
    );
    m_combo_pattern = combo_pattern.get();
    add_row("Pattern", std::move(combo_pattern));
}

void SidebarPrint::add_separator()
{
    Separator* separator = emplace_back<Separator>();
    separator->set_margin(Margins(-m_padding.left, gap(), -m_padding.right, gap()));
}

void SidebarPrint::add_row(const std::string& label, std::unique_ptr<Item> control)
{
    Item* row = emplace_back<Item>();
    row->set_gap(5);

    Text* text = row->emplace_back<Text>(label);
    text->set_width_percent(30);
    text->set_self_align(YGAlign::YGAlignCenter);

    control->set_width_percent(70);

    row->append(std::move(control));
}

} // namespace Slic3r::App
