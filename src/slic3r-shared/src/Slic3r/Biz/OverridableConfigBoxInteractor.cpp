///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/OverridableConfigBoxInteractor.hpp"

#include "Slic3r/Biz/OverridableConfigBoxObservableList.hpp"

namespace Slic3r::Biz {

void OverridableConfigBoxInteractor::SetAccessor::set_source(
    std::weak_ptr<OverridableConfigBoxObservableList> config_box_list
)
{
    m_config_box_list = config_box_list;
}

void OverridableConfigBoxInteractor::SetAccessor::set_value(
    const std::string& key,
    const Domain::ConfigValue& value
)
{
    m_config_box_list.lock()->set_value(key, value);
}

void OverridableConfigBoxInteractor::SetAccessor::set_override(const std::string& key, bool enable)
{
    m_config_box_list.lock()->set_override(key, enable);
}

void OverridableConfigBoxInteractor::SetAccessor::set_config_box(Domain::ConfigBox* config_box) {
    m_config_box_list.lock()->set_config_box(config_box);
}

OverridableConfigBoxInteractor::OverridableConfigBoxInteractor(
    SetAccessor& set_accessor,
    Domain::ConfigBox* config_box
) :
    OverridableConfigBoxInteractor()
{
    set_accessor.set_source(m_config_box_list.get());

    m_config_box_list->set_config_box(config_box);
}

OverridableConfigBoxInteractor::OverridableConfigBoxInteractor() :
    m_config_box_list(std::make_shared<OverridableConfigBoxObservableList>())
{}

const Domain::ConfigValue* OverridableConfigBoxInteractor::find(const std::string& name) const
{
    return m_config_box_list->find(name);
}

std::weak_ptr<const OverridableConfigBoxObservableList>
OverridableConfigBoxInteractor::config_box_overridable_list() const
{
    return m_config_box_list.get();
}

std::weak_ptr<OverridableConfigBoxObservableList>
OverridableConfigBoxInteractor::config_box_overridable_list()
{
    return m_config_box_list.get();
}

} // namespace Slic3r::Biz
