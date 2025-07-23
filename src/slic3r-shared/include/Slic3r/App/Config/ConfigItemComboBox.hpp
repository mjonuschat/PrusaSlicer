///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/App/Yoga/Tooltip.hpp"

namespace Slic3r::App {

class ConfigItemComboBox : public Biz::DataObserver<Domain::ConfigItem>, public Yoga::ComboBox
{
public:
    ConfigItemComboBox(size_t index, const Domain::ConfigItem& config_item);

protected:
    void on_data_update() override;
};

} // namespace Slic3r::App
