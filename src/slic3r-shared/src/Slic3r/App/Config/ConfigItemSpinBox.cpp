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

    on_data_update();

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
}

int ConfigItemSpinBox::value() const
{
    return m_value_validator->value();
}

void ConfigItemSpinBox::on_data_update()
{
    if (*m_state->def().type == typeid(int)) {
        set_text(std::to_string(m_state->get<int>()));
    } else if (*m_state->def().type == typeid(std::optional<int>)) {
        std::optional<int> val = m_state->get<std::optional<int>>();
        set_enabled(val.has_value());
        if (val.has_value()) {
            set_text(std::to_string(val.value()));
        }
    }
}

} // namespace Slic3r::App
