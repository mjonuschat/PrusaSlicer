#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Domain/Preset/Bundle.hpp"
#include "Slic3r/IdGenerator.hpp"

namespace Slic3r::Domain {

class Workbench
{
public:
    using ProjectMap = std::unordered_map<SelectionId, Project>;

    Workbench();
    Workbench(Workbench&&) = default;

    Workbench(const Workbench&) = delete;
    Workbench& operator=(const Workbench&) = delete;

    [[nodiscard]] ProjectMap& projects() { return m_projects; }
    [[nodiscard]] const ProjectMap& projects() const { return m_projects; }

    [[nodiscard]] Project& project(const size_t project_id) { return m_projects.find(project_id)->second; }
    [[nodiscard]] const Project& project(const size_t project_id) const { return m_projects.find(project_id)->second; }

    [[nodiscard]] const PresetBundle& preset_bundle_legacy() const { return *m_preset_bundle_legacy;}
    [[nodiscard]] PresetBundle& preset_bundle_legacy() { return *m_preset_bundle_legacy;}

    [[nodiscard]] const Preset::Bundle& preset_bundle() const { return *m_preset_bundle; }
    [[nodiscard]] Preset::Bundle& preset_bundle() { return  *m_preset_bundle; }

    void set_preset_bundle(Preset::Bundle&& preset_bundle)
    { m_preset_bundle = std::make_unique<Preset::Bundle>(std::forward<Preset::Bundle>(preset_bundle)); }

    void load_legacy_configs();

    SelectionId next_project_id() { return m_project_id_generator.next_id(); }
private:
    ProjectMap m_projects;
    IdGenerator<SelectionId> m_project_id_generator;
    // this is old preset bundle
    std::unique_ptr<PresetBundle> m_preset_bundle_legacy;
    std::shared_ptr<Preset::Bundle> m_preset_bundle;
    std::unique_ptr<AppConfig> m_app_config;
};

} // namespace Slic3r::Domain
