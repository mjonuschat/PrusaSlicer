///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/PrintToolConfigBoxInteractor.hpp"

#include "Slic3r/Biz/PrintToolConfigObservableList.hpp"

namespace Slic3r::Biz {

PrintToolConfigBoxInteractor::PrintToolConfigBoxInteractor(
    const Domain::Workbench& workbench,
    Scene::SceneInteractor& scene_interactor,
    SetAccessor& set_accessor
) :
    m_observable_list(std::make_shared<PrintToolConfigObservableList>(workbench, scene_interactor))
{
    set_accessor.set_source(m_observable_list.get());
}

const Domain::ConfigValue* PrintToolConfigBoxInteractor::find_print_value(
    const std::string& name
) const
{
    return m_observable_list->find_print_value(name);
}

const Domain::ConfigValue*
PrintToolConfigBoxInteractor::find_tool_value(const std::string& name, size_t index) const
{
    return m_observable_list->find_tool_value(name, index);
}

std::weak_ptr<PrintToolConfigObservableList>
PrintToolConfigBoxInteractor::observable_list()
{
    return m_observable_list.get();
}

std::weak_ptr<const PrintToolConfigObservableList>
PrintToolConfigBoxInteractor::observable_list() const
{
    return m_observable_list.get();
}

void PrintToolConfigBoxInteractor::SetAccessor::set_source(
    std::weak_ptr<PrintToolConfigObservableList> observable_list
)
{
    m_observable_list = observable_list;
}

void PrintToolConfigBoxInteractor::SetAccessor::set_print_value(
    const std::string& key,
    const Domain::ConfigValue& value
)
{
    m_observable_list.lock()->set_print_value(key, value);
}

void PrintToolConfigBoxInteractor::SetAccessor::set_tool_override(
    const std::string& key,
    size_t index,
    bool override
)
{
    m_observable_list.lock()->set_tool_override(key, index, override);
}

void PrintToolConfigBoxInteractor::SetAccessor::set_tool_value(
    const std::string& key,
    size_t index,
    const Domain::ConfigValue& value
)
{
    m_observable_list.lock()->set_tool_value(key, index, value);
}

void PrintToolConfigBoxInteractor::SetAccessor::set_project_id(
    Domain::SelectionId selected_project_id
)
{
    m_observable_list.lock()->set_project_id(selected_project_id);
}

void PrintToolConfigBoxInteractor::SetAccessor::set_sources(
    const Domain::SelectionId selected_project_id,
    Domain::ConfigBox* print_config_box,
    const std::vector<Domain::ConfigBox*>& tool_config_boxes
)
{
    m_observable_list.lock()->set_sources(selected_project_id, print_config_box, tool_config_boxes);
}

} // namespace Slic3r::Biz
