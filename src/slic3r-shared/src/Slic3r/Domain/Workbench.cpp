#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/Model.hpp"

#include "libslic3r/AppConfig.hpp"
#include "libslic3r/PresetBundle.hpp"

#include <memory>

namespace Slic3r::Domain {


Workbench::Workbench() : m_project_id_generator(0) {}


void Workbench::load_legacy_configs()
{
    m_app_config = std::make_unique<AppConfig>(Slic3r::AppConfig::EAppMode::Editor);
    m_app_config->load();
    m_preset_bundle_legacy = std::make_unique<Slic3r::PresetBundle>();
    m_preset_bundle_legacy->setup_directories();
    m_preset_bundle_legacy->load_presets(*m_app_config, ForwardCompatibilitySubstitutionRule::EnableSystemSilent);
}

}
