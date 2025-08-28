///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/App/Yoga/InputTextWithSpin.hpp"
#include "Slic3r/Biz/DataObserver.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class IntValidator;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemSpinBox : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::InputTextWithSpin
{
public:
    ConfigItemSpinBox(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::Preset::PresetInteractor& preset_interactor
    );

    int value() const;

protected:
    void on_data_update() override;

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;
    Yoga::IntValidator* m_value_validator{nullptr};
};

} // namespace Slic3r::App
