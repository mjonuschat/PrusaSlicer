///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"

namespace Slic3r::Biz {

ConfigBoxObservableList& ConfigBoxInteractor::config_box_list()
{
    return m_config_box_list;
}

ConfigBoxOverridesObservableList& ConfigBoxInteractor::config_box_overrides_list()
{
    return m_config_box_overrides_list;
}

void ConfigBoxInteractor::set_value(const std::string& key, const Domain::ConfigValue& value)
{
    m_config_box_list.set_value(key, value);
    m_config_box_overrides_list.set_value(key, value);
}

void ConfigBoxInteractor::set_config_box(Domain::ConfigBox* config_box)
{
    m_config_box_list.set_config_box(config_box);
    m_config_box_overrides_list.set_config_box(config_box);
}

} // namespace Slic3r::Biz
