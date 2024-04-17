#include "DataManager.hpp"

#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/PrintConfig.hpp"


DataManager::DataManager()
{
    Slic3r::set_data_dir("/Users/jan.bartipan/Library/Application Support/PrusaSlicer-alpha");
    try {
        Slic3r::AppConfig app_config {Slic3r::AppConfig::EAppMode::Editor};

        try {
            preset_bundle = std::make_unique<Slic3r::PresetBundle>();
            preset_bundle->setup_directories();
            preset_bundle->load_presets(app_config, Slic3r::Disable);
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }

        config = preset_bundle->full_config();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

