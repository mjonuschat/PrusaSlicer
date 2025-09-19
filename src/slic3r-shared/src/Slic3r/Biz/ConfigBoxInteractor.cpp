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
    set_accessor.set_source(
        m_config_box_list.get(),
        m_config_box_overrides_list.get(),
        [&](Domain::ConfigBox* config_box) { set_config_box(config_box); }
    );
    set_config_box(config_box);
}

const Domain::ConfigValue* ConfigBoxInteractor::find(const std::string& name) const
{
    return m_config_box_list->find(name);
}

std::weak_ptr<ConfigBoxObservableList> ConfigBoxInteractor::config_box_list()
{
    return m_config_box_list.get();
}

std::weak_ptr<ConfigBoxOverridesObservableList> ConfigBoxInteractor::config_box_overrides_list()
{
    return m_config_box_overrides_list.get();
}

void ConfigBoxInteractor::set_config_box(Domain::ConfigBox* config_box)
{
    m_config_box_list->set_config_box(config_box);
    m_config_box_overrides_list->set_config_box(config_box);
}

void ConfigBoxInteractor::SetAccessor::set_source(
    std::weak_ptr<ConfigBoxObservableList> config_box_list,
    std::weak_ptr<ConfigBoxOverridesObservableList> config_box_overrides_list,
    ConfigBoxSetter config_box_setter
)
{
    m_config_box_list           = config_box_list;
    m_config_box_overrides_list = config_box_overrides_list;
    m_config_box_setter         = std::move(config_box_setter);
}

void ConfigBoxInteractor::SetAccessor::set_value(const std::string& key, const Domain::ConfigValue& value)
{
    m_config_box_list.lock()->set_value(key, value);
    m_config_box_overrides_list.lock()->set_value(key, value);
}

void ConfigBoxInteractor::SetAccessor::set_override(const std::string& key, bool enable)
{
    m_config_box_overrides_list.lock()->set_override(key, enable);
}

void ConfigBoxInteractor::SetAccessor::set_config_box(Domain::ConfigBox* config_box)
{
    ASSERT(m_config_box_setter != nullptr);
    m_config_box_setter(config_box);
}

} // namespace Slic3r::Biz
