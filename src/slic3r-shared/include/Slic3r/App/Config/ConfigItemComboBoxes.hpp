///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class ComboBox;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemComboBoxes : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::Item
{
public:
    ConfigItemComboBoxes(
        size_t index,
        const Domain::ConfigItem& config_item,
        Biz::Preset::PresetInteractor& preset_interactor
    );

protected:
    void on_data_update() override;

private:
    void reconstruct_buttons();
    void update_values();

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;
    std::vector<Yoga::ComboBox*> m_combo_boxes;
};

} // namespace Slic3r::App
