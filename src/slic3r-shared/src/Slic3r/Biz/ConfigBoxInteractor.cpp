///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"

namespace Slic3r::Biz {

ConfigBoxInteractor::ConfigBoxInteractor() :
    m_config_box_list(std::make_shared<ConfigBoxObservableList>()),
    m_config_box_overrides_list(std::make_shared<ConfigBoxOverridesObservableList>())
{}

ConfigBoxInteractor::ConfigBoxInteractor(SetAccessor& set_accessor, Domain::ConfigBox* config_box) :
    ConfigBoxInteractor()
{
    set_accessor.set_source(m_config_box_list, m_config_box_overrides_list);
    set_config_box(config_box);
}

std::weak_ptr<ConfigBoxObservableList> ConfigBoxInteractor::config_box_list()
{
    return m_config_box_list;
}

std::weak_ptr<ConfigBoxOverridesObservableList> ConfigBoxInteractor::config_box_overrides_list()
{
    return m_config_box_overrides_list;
}

void ConfigBoxInteractor::set_config_box(Domain::ConfigBox* config_box)
{
    m_config_box_list->set_config_box(config_box);
    m_config_box_overrides_list->set_config_box(config_box);
}

void ConfigBoxInteractor::SetAccessor::set_source(
    std::weak_ptr<ConfigBoxObservableList> config_box_list,
    std::weak_ptr<ConfigBoxOverridesObservableList> config_box_overrides_list
)
{
    m_config_box_list           = config_box_list;
    m_config_box_overrides_list = config_box_overrides_list;
}

void ConfigBoxInteractor::SetAccessor::set_value(const std::string& key, const Domain::ConfigValue& value)
{
    m_config_box_list.lock()->set_value(key, value);
    m_config_box_overrides_list.lock()->set_value(key, value);
}

} // namespace Slic3r::Biz
