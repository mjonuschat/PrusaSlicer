///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class InputTextField;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemTextFields : public ConfigItemControl, public Yoga::Item
{
public:
    ConfigItemTextFields(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::Preset::PresetInteractor& preset_interactor,
        size_t cbi_index
    );

protected:
    void on_data_update() override;

private:
    void reconstruct_fields();
    void update_values();
    void send_data();

private:
    struct Field {
        Yoga::InputTextField* textfield{nullptr};
        Yoga::Passthrough<Yoga::DoubleValidator> double_validator;
    };

    Biz::Preset::PresetInteractor& m_preset_interactor;
    size_t m_cbi_index{0};
    std::vector<Field> m_fields;
};

} // namespace Slic3r::App
