#pragma once

#include <memory>
#include <vector>
#include <string>
#include "Project.hpp"

#include <libslic3r/PresetBundle.hpp>
#include <libslic3r/AppConfig.hpp>

namespace Slic3r::Domain {

class Workbench
{
public:
    using ProjectList = std::vector<Project>;

    Workbench() = default;
    Workbench(Workbench&&) = default;
    Workbench& operator=(Workbench&&) = default;

    Workbench(const Workbench&) = delete;
    Workbench& operator=(const Workbench&) = delete;

    [[nodiscard]] ProjectList& projects() { return m_projects; }
    [[nodiscard]] const ProjectList& projects() const { return m_projects; }

    [[nodiscard]] Project& project(const size_t project_id) { return m_projects[project_id]; }
    [[nodiscard]] const Project& project(const size_t project_id) const { return m_projects[project_id]; }

    [[nodiscard]] const PresetBundle& preset_bundle() const { return *m_preset_bundle;}
    [[nodiscard]] PresetBundle& preset_bundle() { return *m_preset_bundle;}

    void load_configs();
    void load_project(const std::string& file_path);

private:
    ProjectList m_projects;
    std::unique_ptr<PresetBundle> m_preset_bundle;
    std::unique_ptr<AppConfig> m_app_config;
};

} // namespace Slic3r::Domain
