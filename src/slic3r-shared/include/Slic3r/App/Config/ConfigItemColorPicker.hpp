///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/ColorPickerButton.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"

namespace Slic3r::App {

class ConfigItemColorPicker : public ConfigItemControl, public Yoga::ColorPickerButton
{
public:
    ConfigItemColorPicker(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::IConfigBoxSetter& cbi_container,
        size_t cbi_index
    );

protected:
    void on_data_update() override;

private:
    Biz::IConfigBoxSetter& m_cbi_container;
    size_t m_cbi_index{0};
};

} // namespace Slic3r::App
