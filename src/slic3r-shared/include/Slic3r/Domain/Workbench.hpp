#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/IdGenerator.hpp"

#include <libslic3r/PresetBundle.hpp>
#include <libslic3r/AppConfig.hpp>

namespace Slic3r::Domain {

class Workbench
{
public:
    using ProjectMap = std::unordered_map<SelectionId, Project>;

    Workbench();
    Workbench(Workbench&&) ;

    Workbench(const Workbench&) = delete;
    Workbench& operator=(const Workbench&) = delete;

    [[nodiscard]] ProjectMap& projects() { return m_projects; }
    [[nodiscard]] const ProjectMap& projects() const { return m_projects; }

    [[nodiscard]] Project& project(const size_t project_id) { return m_projects.find(project_id)->second; }
    [[nodiscard]] const Project& project(const size_t project_id) const { return m_projects.find(project_id)->second; }

    [[nodiscard]] const PresetBundle& preset_bundle() const { return *m_preset_bundle;}
    [[nodiscard]] PresetBundle& preset_bundle() { return *m_preset_bundle;}

    void load_configs();
    void load_project(const std::string& file_path);

    SelectionId next_project_id() { return m_project_id_generator.next_id(); }
private:
    ProjectMap m_projects;
    IdGenerator<SelectionId> m_project_id_generator;
    std::unique_ptr<PresetBundle> m_preset_bundle;
    std::unique_ptr<AppConfig> m_app_config;
};

} // namespace Slic3r::Domain
