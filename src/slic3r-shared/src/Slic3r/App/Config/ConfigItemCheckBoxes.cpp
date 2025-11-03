///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemCheckBoxes.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

#include "Slic3r/App/Yoga/ToggleButton.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemCheckBoxes::ConfigItemCheckBoxes(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor,
    size_t cbi_index
) :
    ConfigItemControl(index, data),
    m_preset_interactor(preset_interactor),
    m_cbi_index(cbi_index)
{
    on_data_update();

    set_orientation(Orientation::Horizontal);
    set_gap(5);

    // m_tooltip.set_text_wrap(true);
    // m_tooltip.content_item()->set_width(350);
    // set_tooltip(data.def().tooltip);
}

void ConfigItemCheckBoxes::on_data_update()
{
    if (m_toggle_buttons.size() != get_data().size()) {
        reconstruct_buttons();
    } else {
        update_values();
    }
}

std::vector<bool> ConfigItemCheckBoxes::get_data() const
{
    return m_state->value().get<std::vector<bool>>();
}

void ConfigItemCheckBoxes::reconstruct_buttons()
{
    for (size_t child_index = 0; child_index < item_count(); ++child_index) {
        remove(get_item(0));
    }
    m_toggle_buttons.clear();

    size_t size = get_data().size();
    m_toggle_buttons.reserve(size);

    for (size_t index = 0; index < size; ++index) {
        ToggleButton* button = emplace_back<ToggleButton>();
        m_toggle_buttons.push_back(button);
        button->callbacks().action = [this, index, button]
        {
            bool checked           = button->checked();
            std::vector<bool> data = get_data();
            data[index]            = checked;
            m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{data}, m_cbi_index);
        };
    }

    update_values();
}

void ConfigItemCheckBoxes::update_values()
{
    const std::vector<bool>& data = get_data();
    for (size_t i = 0; i < data.size(); ++i) {
        m_toggle_buttons.at(i)->set_checked(data.at(i));
    }
}

} // namespace Slic3r::App
