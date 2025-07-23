///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemComboBox.hpp"

#include "Slic3r/App/Yoga/Validator.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemComboBox::ConfigItemComboBox(size_t index, const Domain::ConfigItem& config_item) :
    Biz::DataObserver<Domain::ConfigItem>(index, config_item),
    ComboBox("ConfigItemCombo")
{
    set_editable(
        config_item.def().gui_type == Domain::ConfigItemDef::GUIType::f_enum_open
        || config_item.def().gui_type == Domain::ConfigItemDef::GUIType::i_enum_open
    );

    set_width(150);

    std::vector<std::string> items;

    if (config_item.def().gui_type == Domain::ConfigItemDef::GUIType::f_enum_open) {
        set_editable(true);
        set_validator(std::make_unique<DoubleValidator>());

        for (const auto& choice : config_item.def().choices) {
            items.push_back(choice.second);
        }
    } else if (config_item.def().gui_type == Domain::ConfigItemDef::GUIType::i_enum_open) {
        set_editable(true);
        set_validator(std::make_unique<IntValidator>());

        for (const auto& choice : config_item.def().choices) {
            items.push_back(choice.second);
        }
    } else if (*config_item.def().type == typeid(Domain::EnumWrapper)){
        const Domain::EnumWrapper values = config_item.get<Domain::EnumWrapper>();

        for (const Domain::EnumValueDef& value : values.def()) {
            items.push_back(std::string(value.str_ui));
        }
    }

    set_items(items);

    m_tooltip.content_item()->set_width(350);
    m_tooltip.set_text_wrap(true);
}

void ConfigItemComboBox::on_data_update()
{
    // do something clever
}

} // namespace Slic3r::App
