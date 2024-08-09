#include "Workbench.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Utils/DirectoriesUtils.hpp"

namespace Slic3r::Domain {

void Workbench::load_configs()
{
    m_app_config = std::make_unique<AppConfig>(Slic3r::AppConfig::EAppMode::Editor);
    m_preset_bundle = std::make_unique<Slic3r::PresetBundle>();
    m_preset_bundle->setup_directories();
    m_preset_bundle->load_presets(*m_app_config, Slic3r::Disable);
}

}
