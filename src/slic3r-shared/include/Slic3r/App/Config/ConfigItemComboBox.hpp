///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App {

class ConfigItemComboBox : public ConfigItemControl, public Yoga::ComboBox
{
public:
    ConfigItemComboBox(
        size_t index,
        const Domain::ConfigItem& config_item,
        Biz::Preset::PresetInteractor& preset_interactor,
        size_t cbi_index
    );

protected:
    void on_data_update() override;
    void update_value(const Domain::ConfigValue& value);
    void initialize();

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;
    size_t m_cbi_index = 0;

    Yoga::Passthrough<Yoga::IntValidator> m_int_validator;
    Yoga::Passthrough<Yoga::DoubleValidator> m_double_validator;
    Yoga::Passthrough<Yoga::PercentageValidator> m_percentage_validator;

    bool m_init = false;
};

} // namespace Slic3r::App
