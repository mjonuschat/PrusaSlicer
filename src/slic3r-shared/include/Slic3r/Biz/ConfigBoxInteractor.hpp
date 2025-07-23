///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/ConfigBoxObservableList.hpp"

namespace Slic3r::Biz {

class PresetInteractor;

class ConfigBoxInteractor
{
public:
    ConfigBoxObservableList& config_box_list();

    ConfigBoxOverridesObservableList& config_box_overrides_list();

    void set_value(const std::string& key, const Domain::ConfigValue& value);

    void set_config_box(Domain::ConfigBox* config_box);

private:
    ConfigBoxObservableList m_config_box_list;
    ConfigBoxOverridesObservableList m_config_box_overrides_list;
};

} // namespace Slic3r::Biz
