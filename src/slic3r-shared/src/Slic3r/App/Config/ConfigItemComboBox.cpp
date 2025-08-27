///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemComboBox.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemComboBox::ConfigItemComboBox(
    size_t index,
    const Domain::ConfigItem& config_item,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    ConfigItemControl(index, config_item),
    ComboBox("ConfigItemCombo"),
    m_preset_interactor(preset_interactor)
{
    set_editable(
        config_item.def().gui_type == Domain::ConfigItemDef::GUIType::f_enum_open
        || config_item.def().gui_type == Domain::ConfigItemDef::GUIType::i_enum_open
    );

    set_width(150);

    on_data_update();

    m_tooltip.set_text(tooltip_text());
    m_tooltip.content_item()->set_width(350);
    m_tooltip.set_text_wrap(true);

    callbacks().selection_changed = [this](int selected) {
        if (m_state->def().gui_type == Domain::ConfigItemDef::GUIType::f_enum_open) {
            m_preset_interactor
                .set_item_value(*m_state, Domain::ConfigValue{m_double_validator->value()});
        } else if (m_state->def().gui_type == Domain::ConfigItemDef::GUIType::i_enum_open) {
            m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{m_int_validator->value()});
        } else if (*m_state->def().type == typeid(Domain::EnumWrapper)) {
            Domain::EnumWrapper values = m_state->get<Domain::EnumWrapper>();
            values.set_string(values.def().at(static_cast<size_t>(selected)).str_serialized);
            m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{values});
        }
    };
}

void ConfigItemComboBox::on_data_update()
{
    std::vector<std::string> items;

    if (m_state->def().gui_type == Domain::ConfigItemDef::GUIType::f_enum_open) {
        set_editable(true);
        m_double_validator = std::make_unique<DoubleValidator>();
        set_validator(m_double_validator.release());

        for (const auto& choice : m_state->def().choices) {
            items.push_back(choice.second);
        }
        set_items(items);
        // set_current_label(fmt::format("{}", m_state->get<double>()));

    } else if (m_state->def().gui_type == Domain::ConfigItemDef::GUIType::i_enum_open) {
        set_editable(true);
        m_int_validator = std::make_unique<IntValidator>();
        set_validator(m_int_validator.release());

        for (const auto& choice : m_state->def().choices) {
            items.push_back(choice.second);
        }
        set_items(items);
        set_current_label(std::to_string(m_state->get<int>()));

    } else if (*m_state->def().type == typeid(Domain::EnumWrapper)) {
        set_editable(false);
        const Domain::EnumWrapper enum_wrapper = m_state->get<Domain::EnumWrapper>();

        for (const Domain::EnumValueDef& value : enum_wrapper.def()) {
            items.push_back(std::string(value.str_ui));
        }
        set_items(items);
        set_current_index(enum_wrapper.index_of_value(enum_wrapper.value()));
    }
}

} // namespace Slic3r::App
