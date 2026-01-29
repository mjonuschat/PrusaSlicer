///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Domain/ConfigDef.hpp>

#include "Slic3r/Biz/Platform/ListenerScope.hpp"
#include "Slic3r/Biz/PrintToolItem.hpp"
#include "Slic3r/Biz/IObservableList.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ISelectedBedInstanceChangedListener.hpp"

namespace Slic3r::Biz {

class PrintToolConfigObservableList :
    public IObservableList<PrintToolItem>,
    public Scene::ISceneBedInstanceChangedListener,
    public ISelectedBedInstancesChangedListener
{
public:
    explicit PrintToolConfigObservableList(
        const Domain::Workbench& workbench,
        Scene::SceneInteractor& scene_interactor
    );

    const PrintToolItem& at(size_t index) const override;

    size_t size() const override;

    /**
     * @note this will either invoke on_reset or just data_changed
     * in order to decide which, we want to update all sources at the same time
     */
    void set_sources(
        const Domain::SelectionId selected_project_id,
        Domain::ConfigBox* print_config_box,
        const std::vector<Domain::ConfigBox*>& tool_config_boxes
    );

    void set_print_value(const std::string& key, const Domain::ConfigValue& value);

    void set_tool_override(const std::string& key, size_t index, bool override);

    void set_tool_value(const std::string& key, size_t index, const Domain::ConfigValue& value);

    void set_project_id(const Domain::SelectionId selected_project_id);

    const Domain::ConfigValue* find_print_value(const std::string& name) const;

    const Domain::ConfigValue* find_tool_value(const std::string& name, size_t index) const;

    void on_bed_instance_extruder_candidates_changed(
        Domain::SelectionId project_id,
        Domain::BedRef instance,
        const std::vector<unsigned int>& extruder_candidates
    ) override;

    void on_selected_bed_instances_changed(
        Domain::SelectionId project_id,
        const Scene::BedSelection& bed_selection
    ) override;

private:
    using ToolPrintItems = std::vector<PrintToolItem>;

    ToolPrintItems::iterator find_item(const std::string& name);

    void update_items();
    void update_extruders();

private:
    const Domain::Workbench& m_workbench;
    Scene::SceneInteractor& m_scene_interactor;

    ListenerScope<
        Scene::ISceneBedInstanceChangedListener,
        Scene::SceneInteractor,
        PrintToolConfigObservableList>
        m_scene_bed_changed_listener_scope;
    ListenerScope<
        ISelectedBedInstancesChangedListener,
        Scene::SceneInteractor,
        PrintToolConfigObservableList>
        m_selected_bed_changed_listener_scope;

    Domain::SelectionId m_selected_project_id = Domain::INVALID_ID;
    Domain::ConfigBox* m_print_config_box{nullptr};
    std::vector<Domain::ConfigBox*> m_tool_config_boxes;
    ToolPrintItems m_items;
    std::set<unsigned> m_extruder_candidates;
};

} // namespace Slic3r::Biz
