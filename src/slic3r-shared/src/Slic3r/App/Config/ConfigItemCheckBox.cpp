///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemCheckBox.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

namespace Slic3r::App {

ConfigItemCheckBox::ConfigItemCheckBox(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    Biz::DataObserver<Domain::ConfigItem>(index, data),
    m_preset_interactor(preset_interactor)
{
    on_data_update();

    set_width(150);
    m_tooltip.set_text_wrap(true);
    m_tooltip.content_item()->set_width(350);
    set_tooltip(data.def().tooltip);
}

void ConfigItemCheckBox::on_data_update()
{
    set_checked(m_state->value().get<bool>());
}

void ConfigItemCheckBox::checked_updated_internal()
{
    ToggleButton::checked_updated_internal();

    if (m_state->value().get<bool>() != checked()) {
        m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{checked()});
    }
}

} // namespace Slic3r::App
