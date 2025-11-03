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

namespace Slic3r::App {

class ConfigItemSubstitutions : public ConfigItemControl, public Yoga::Item
{
public:
    ConfigItemSubstitutions(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::Preset::PresetInteractor& preset_interactor,
        size_t cbi_index
    );

protected:
    void on_data_update() override;

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;
    size_t m_cbi_index{0};
};

} // namespace Slic3r::App
