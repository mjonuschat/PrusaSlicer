///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/Domain/SelectionId.hpp"

#include "Slic3r/Biz/IListObserver.hpp"

#include <vector>
#include <string>
#include <memory>

namespace Slic3r::Domain {
struct ConfigBox;
struct ConfigValue;
class Workbench;
} // namespace Slic3r::Domain

namespace Slic3r::Domain::Preset {
struct SelectedPreset;
}

namespace Slic3r::Biz::Scene {
class SceneInteractor;
} // namespace Slic3r::Biz::Scene

namespace Slic3r::Biz {

class PrintToolConfigObservableList;

class PrintToolConfigBoxInteractor
{
public:
    class SetAccessor
    {
    public:
        void set_source(std::weak_ptr<PrintToolConfigObservableList> observable_list);

        void set_print_value(const std::string& key, const Domain::ConfigValue& value);

        void set_tool_override(const std::string& key, size_t index, bool override);

        void set_tool_value(const std::string& key, size_t index, const Domain::ConfigValue& value);

        void set_project_id(Domain::SelectionId selected_project_id);

        void set_sources(
            const Domain::SelectionId selected_project_id,
            Domain::Preset::SelectedPreset& selected_preset,
            const std::vector<Domain::ConfigBox*>& tool_config_boxes
        );

    private:
        std::weak_ptr<PrintToolConfigObservableList> m_observable_list;
    };

    explicit PrintToolConfigBoxInteractor(
        const Domain::Workbench& workbench,
        Scene::SceneInteractor& scene_interactor,
        SetAccessor& set_accessor
    );

    const Domain::ConfigValue* find_print_value(const std::string& name) const;

    const Domain::ConfigValue* find_tool_value(const std::string& name, size_t index) const;

    std::weak_ptr<PrintToolConfigObservableList> observable_list();

    std::weak_ptr<const PrintToolConfigObservableList> observable_list() const;

private:
    Biz::UnsharedPointer<PrintToolConfigObservableList> m_observable_list;
};

} // namespace Slic3r::Biz
