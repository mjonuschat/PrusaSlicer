///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"

namespace Slic3r::App {

class ConfigItemCheckBox : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::ToggleButton {
public:

    ConfigItemCheckBox(size_t index, const Domain::ConfigItem& data);

protected:
    void on_data_update() override;
};

}
