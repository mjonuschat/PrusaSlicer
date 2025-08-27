///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"

namespace Slic3r::App {

class ConfigItemColorPicker : public ConfigItemControl, public Yoga::Rectangle
{
public:
    ConfigItemColorPicker(size_t index, const Domain::ConfigItem& data);

protected:
    void on_data_update() override;

private:
};

} // namespace Slic3r::App
