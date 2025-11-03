///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App {

class ConfigItemTextField : public ConfigItemControl, public Yoga::InputTextField
{
public:
    ConfigItemTextField(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::Preset::PresetInteractor& preset_interactor,
        size_t cbi_index
    );

protected:
    void on_data_update() override;
    void update_value(const Domain::ConfigValue& value);

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;
    size_t m_cbi_index{0};
    Yoga::Passthrough<Yoga::DoubleValidator> m_double_validator;
    Yoga::Passthrough<Yoga::PercentageValidator> m_percentage_validator;
};

} // namespace Slic3r::App
