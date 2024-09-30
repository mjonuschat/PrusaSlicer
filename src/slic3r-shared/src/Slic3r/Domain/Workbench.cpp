#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "libslic3r/Utils/DirectoriesUtils.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r::Domain {

void Workbench::load_configs()
{
    m_app_config = std::make_unique<AppConfig>(Slic3r::AppConfig::EAppMode::Editor);
    m_app_config->load();
    m_preset_bundle = std::make_unique<Slic3r::PresetBundle>();
    m_preset_bundle->setup_directories();
    m_preset_bundle->load_presets(*m_app_config, ForwardCompatibilitySubstitutionRule::EnableSystemSilent);
}

void Workbench::load_project(const std::string& file_path)
{
    Project project;
    project.load(file_path);
    m_projects.emplace_back(std::move(project));
}

}
