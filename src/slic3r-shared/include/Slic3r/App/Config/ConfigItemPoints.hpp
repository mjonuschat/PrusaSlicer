///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

namespace Slic3r::Biz::Preset {
class PresetInteractor;
} // namespace Slic3r::Biz::Preset

namespace Slic3r::App::Yoga {
class InputTextField;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemPoints : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::Item
{
public:
    ConfigItemPoints(size_t index, const Domain::ConfigItem& data, Biz::Preset::PresetInteractor& preset_interactor);

protected:
    void on_data_update() override;
    void send_data();

private:
    Biz::Preset::PresetInteractor& m_preset_interactor;
    Yoga::InputTextField* m_input_x{nullptr};
    Yoga::InputTextField* m_input_y{nullptr};
    Yoga::Passthrough<Yoga::DoubleValidator> m_validator_x;
    Yoga::Passthrough<Yoga::DoubleValidator> m_validator_y;
};

} // namespace Slic3r::App
