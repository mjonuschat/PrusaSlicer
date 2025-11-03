///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class ToggleButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemCheckBoxes : public ConfigItemControl, public Yoga::Item
{
public:
    ConfigItemCheckBoxes(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::Preset::PresetInteractor& preset_interactor,
        size_t cbi_index
    );

protected:
    void on_data_update() override;

    std::vector<bool> get_data() const;

private:
    void reconstruct_buttons();
    void update_values();

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;
    size_t m_cbi_index{0};
    std::vector<Yoga::ToggleButton*> m_toggle_buttons;
};

} // namespace Slic3r::App
