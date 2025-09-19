///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/ObjectSettingsObservableList.hpp"

namespace Slic3r::Biz {

namespace Scene {
class SceneInteractor;
} // namespace Scene

class ObjectSettingsInteractor :
    public Scene::ISceneSelectionChangedListener,
    public ISelectedConfigContainerChangedListener
{
public:
    class SetAccessor
    {
    public:
        void set_source(std::weak_ptr<ObjectSettingsObservableList> object_observable_list);

        void set_value(const std::string& key, const Domain::ConfigValue& value);
        void set_override(const std::string& key, bool enable);

    private:
        std::weak_ptr<ObjectSettingsObservableList> m_object_observable_list;
    };

    explicit ObjectSettingsInteractor(
        SetAccessor& set_accessor,
        Domain::Workbench& workbench,
        Scene::SceneInteractor& scene_interactor
    );

    std::weak_ptr<ObjectSettingsObservableList> object_observable_list() const;

    void on_scene_selection_changed(
        Domain::SelectionId project_id,
        const Scene::ObjectSelection& selection
    ) override;

    void on_selected_config_container_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId container_id
    ) override;

private:
    void update_sources();

private:
    Domain::Workbench& m_workbench;
    Scene::SceneInteractor& m_scene_interactor;

    Domain::PrinterTechnology m_current_print_technology = Domain::PrinterTechnology::FFF;
    Scene::ObjectSelection m_current_selection;
    Domain::SelectionId m_project_id = Domain::INVALID_ID;
    UnsharedPointer<ObjectSettingsObservableList> m_object_observable_list;
};

} // namespace Slic3r::Biz
