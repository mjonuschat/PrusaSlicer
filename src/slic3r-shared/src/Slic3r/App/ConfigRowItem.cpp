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

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigRowItem::ConfigRowItem(size_t index, const Domain::ConfigItem& data) :
    Biz::DataObserver<Domain::ConfigItem>(index, data)
{
    set_flex_shrink(0);

    m_label = emplace_back<Text>(data.def().label);
    m_label->set_width(175);
    m_label->set_wrap(true);

    switch (data.def().gui_type) {
    case Slic3r::Domain::ConfigItemDef::GUIType::textfield:
        m_input = emplace_back<ConfigItemTextField>(index, data);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::textfields:
        m_input = emplace_back<ConfigItemTextFields>(index, data);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::checkbox:
        m_input = emplace_back<ConfigItemCheckBox>(index, data);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::f_enum_open:
    case Slic3r::Domain::ConfigItemDef::GUIType::i_enum_open:
    case Slic3r::Domain::ConfigItemDef::GUIType::combobox:
        m_input = emplace_back<ConfigItemComboBox>(index, data);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::points:
        m_input = emplace_back<ConfigItemPoints>(index, data);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::color:
        m_input = emplace_back<ConfigItemColorPicker>(index, data);
        break;
    case Slic3r::Domain::ConfigItemDef::GUIType::spinbox:
        m_input = emplace_back<ConfigItemSpinBox>(index, data);
        break;
    }

    if (m_input) { // Todo: handle all cases
        if (data.def().full_width) {
            m_input->set_flex_grow(1);
            set_orientation(Orientation::Vertical);
        } else {
            set_align_items(YGAlign::YGAlignCenter);
        }
        m_input_value = dynamic_cast<Biz::DataObserver<Domain::ConfigItem>*>(m_input);
    }

    m_sidetext = emplace_back<Text>(data.def().sidetext);
}

void ConfigRowItem::on_data_update()
{
    m_label->set_text(m_state->name());
    m_sidetext->set_text(m_state->def().sidetext);
    if (m_input_value) { // Todo: handle all cases
        m_input_value->set_state(*m_state);
    }
}

} // namespace Slic3r::App
