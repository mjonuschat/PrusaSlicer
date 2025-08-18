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
    Biz::DataObserver<Domain::ConfigItem>(index, data),
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
        m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{m_value_validator->value()});
    };

    set_tooltip(data.def().tooltip);
    m_tooltip.set_text_wrap(true);
    m_tooltip.content_item()->set_width(350);
}

void ConfigItemSpinBox::on_data_update()
{
    set_text(std::to_string(m_state->get<int>()));
}

} // namespace Slic3r::App
