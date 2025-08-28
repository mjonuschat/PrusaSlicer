///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class Text;
class ToggleButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemSpinBox;

class ConfigRowItem : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::Item
{
public:
    ConfigRowItem(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::Preset::PresetInteractor& preset_interactor
    );

private:
    void on_data_update() override;

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;

    Yoga::Text* m_label{nullptr};
    Yoga::Text* m_sidetext{nullptr};
    Yoga::Item* m_input{nullptr};
    Yoga::ToggleButton* m_toggle_enable{nullptr};

    ConfigItemSpinBox* m_config_item_spin_box{nullptr}; ///< valid only if ConfigItem gui type is spinbox

    Biz::DataObserver<Domain::ConfigItem>* m_input_value{nullptr};
};

} // namespace Slic3r::App
