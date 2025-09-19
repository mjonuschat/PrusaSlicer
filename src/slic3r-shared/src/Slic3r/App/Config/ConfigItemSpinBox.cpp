///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemSpinBox.hpp"

#include "Slic3r/App/Yoga/Validator.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemSpinBox::ConfigItemSpinBox(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    ConfigItemControl(index, data),
    InputTextWithSpin(
        std::make_unique<IntValidator>(
            data.def().min.value_or(std::numeric_limits<int>::min()),
            data.def().max.value_or(std::numeric_limits<int>::max())
        )
    ),
    m_preset_interactor(preset_interactor)
{
    m_value_validator = dynamic_cast<IntValidator*>(validator());

    callbacks().text_edited = [this]() {
        if (*m_state->def().type == typeid(int)) {
            m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{value()});
        } else if (*m_state->def().type == typeid(std::optional<int>)) {
            if (m_state->get<std::optional<int>>().has_value()) {
                m_preset_interactor
                    .set_item_value(*m_state, Domain::ConfigValue{std::optional<int>(value())});
            }
        }
    };

    set_tooltip(tooltip_text());
    m_tooltip.set_text_wrap(true);
    m_tooltip.content_item()->set_width(350);

    on_data_update();
}

int ConfigItemSpinBox::value() const
{
    return m_value_validator->value();
}

void ConfigItemSpinBox::on_data_update()
{
    if (mixed()) {
        set_override_label("Mixed");
        set_font_type(Render::ImguiFontType::Italic);
        return;
    }

    set_override_label({});
    set_font_type(Render::ImguiFontType::Regular);
    if (!overriden().value_or(true)) {
        update_value(*m_preset_interactor.get_override_origin(*m_state, location_index()));
    } else {
        update_value(m_state->value());
    }
}

void ConfigItemSpinBox::update_value(const Domain::ConfigValue& value)
{
    if (*m_state->def().type == typeid(int)) {
        set_text(std::to_string(value.get<int>()));
    } else if (*m_state->def().type == typeid(std::optional<int>)) {
        std::optional<int> val = value.get<std::optional<int>>();
        set_enabled(val.has_value());
        if (val.has_value()) {
            set_text(std::to_string(val.value()));
        }
    }
}

} // namespace Slic3r::App
