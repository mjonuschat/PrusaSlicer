///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/App/Yoga/InputTextWithSpin.hpp"
#include "Slic3r/Biz/DataObserver.hpp"

namespace Slic3r::App {

class ConfigItemSpinBox : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::InputTextWithSpin
{
public:
    ConfigItemSpinBox(size_t index, const Domain::ConfigItem& data);

protected:
    void on_data_update() override;

private:
};

} // namespace Slic3r::App
