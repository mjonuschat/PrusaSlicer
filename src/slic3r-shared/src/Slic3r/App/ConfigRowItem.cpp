///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ConfigRowItem.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Config/ConfigItemTextField.hpp"
#include "Slic3r/App/Config/ConfigItemCheckBox.hpp"
#include "Slic3r/App/Config/ConfigItemColorPicker.hpp"
#include "Slic3r/App/Config/ConfigItemPoints.hpp"
#include "Slic3r/App/Config/ConfigItemComboBox.hpp"
#include "Slic3r/App/Config/ConfigItemTextFields.hpp"
#include "Slic3r/App/Config/ConfigItemSpinBox.hpp"
#include "Slic3r/App/Config/ConfigItemSpinBoxes.hpp"
#include "Slic3r/App/Config/ConfigItemComboBoxes.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigRowItem::ConfigRowItem(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor,
    bool small
) :
    Biz::DataObserver<Domain::ConfigItem>(index, data),
    m_preset_interactor(preset_interactor)
{
    set_flex_shrink(0);

    Item* left_side = emplace_back<Item>();

    if (*m_state->def().type == typeid(std::optional<int>)) {
        m_toggle_enable = left_side->emplace_back<ToggleButton>();
        m_toggle_enable->set_margin(Margins(0, 0, 5, 0));

        std::optional<int> value = m_state->value().get<std::optional<int>>();
        m_toggle_enable->set_checked(value.has_value());
        m_toggle_enable->callbacks().action = [this]() {
            // We are using action to make sure this callbacks comes from user
            std::optional<int> value;
            if (m_toggle_enable->checked()) {
                value = m_config_item_spin_box->value();
            }

            m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{value});
        };
    }

    m_label = left_side->emplace_back<Text>(data.def().label);

    switch (data.def().gui_type) {
    case Slic3r::Domain::ConfigItemDef::GUIType::textfield:
        m_input = emplace_back<ConfigItemTextField>(index, data, m_preset_interactor);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::textfields:
        m_input = emplace_back<ConfigItemTextFields>(index, data, m_preset_interactor);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::checkbox:
        m_input = emplace_back<ConfigItemCheckBox>(index, data, m_preset_interactor);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::f_enum_open:
    case Slic3r::Domain::ConfigItemDef::GUIType::i_enum_open:
    case Slic3r::Domain::ConfigItemDef::GUIType::combobox:
        m_input = emplace_back<ConfigItemComboBox>(index, data, m_preset_interactor);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::comboboxes:
        m_input = emplace_back<ConfigItemComboBoxes>(index, data, m_preset_interactor);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::points:
        m_input = emplace_back<ConfigItemPoints>(index, data);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::color:
        m_input = emplace_back<ConfigItemColorPicker>(index, data);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::spinbox:
        m_input = m_config_item_spin_box = emplace_back<ConfigItemSpinBox>(
            index,
            data,
            m_preset_interactor
        );
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::spinboxes:
        m_input = emplace_back<ConfigItemSpinBoxes>(index, data, m_preset_interactor);
        break;
    }

    if (m_input) { // Todo: handle all cases
        m_input_value = dynamic_cast<Biz::DataObserver<Domain::ConfigItem>*>(m_input);
    }

    m_sidetext = emplace_back<Text>(data.def().sidetext);
    m_sidetext->set_self_align(YGAlignCenter);

    if (small) {
        set_gap(5);
        set_width(175);
        m_input->set_min_size({50, YGUndefined});

        left_side->set_flex_grow(1);
        m_label->set_self_align(YGAlignCenter);
    } else {
        m_label->set_flex_grow(1);
        m_label->set_wrap(true);

        if (data.def().full_width) {
            m_input->set_flex_grow(1);
            set_orientation(Orientation::Vertical);
        } else {
            set_align_items(YGAlign::YGAlignCenter);
        }

        left_side->set_width(175);
        left_side->set_max_size({175, YGUndefined});
    }
}

void ConfigRowItem::on_data_update()
{
    m_label->set_text(m_state->def().label);
    m_sidetext->set_text(m_state->def().sidetext);

    if (*m_state->def().type == typeid(std::optional<int>)) {
        std::optional<int> value = m_state->value().get<std::optional<int>>();
        m_toggle_enable->set_checked(value.has_value());
    }

    if (m_input_value) {
        m_input_value->set_state(*m_state);
    }
}

} // namespace Slic3r::App
