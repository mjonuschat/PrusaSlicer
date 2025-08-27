///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class IntValidator;
class InputTextWithSpin;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemSpinBoxes : public ConfigItemControl, public Yoga::Item
{
public:
    ConfigItemSpinBoxes(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::Preset::PresetInteractor& preset_interactor
    );

protected:
    void on_data_update() override;

private:
    void reconstruct_spin_buttons();
    void update_values();

private:
    struct Box
    {
        Yoga::InputTextWithSpin* spinbox{nullptr};
        Yoga::IntValidator* value_validator{nullptr};
    };

    Biz::Preset::PresetInteractor& m_preset_interactor;

    std::vector<Box> m_boxes;
};

} // namespace Slic3r::App
