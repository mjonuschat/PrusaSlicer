#include "Slic3r/Biz/Preset/IO/BundleLoader.hpp"

#include "Yaml.hpp"
#include "Slic3r/Biz/Preset/IO/PresetLoader.hpp"
#include "Slic3r/Biz/Preset/IO/HwConfigLoader.hpp"
#include "spdlog/spdlog.h"

#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/exception.hpp>

namespace Slic3r::Biz::Preset::IO {

namespace fs = boost::filesystem;

Domain::Preset::Bundle load_bundle(const std::string& bundle_path, const std::string& config_path)
{

    Domain::Preset::Bundle bundle;
    HwConfigLoader config_loader;
    PresetLoader preset_loader;

    fs::path config_base{config_path};

    for (const auto& repo_entry : fs::directory_iterator(bundle_path)) {
        if (!repo_entry.is_directory())
            continue;
        for (const auto& vendor_entry : fs::directory_iterator(repo_entry.path())) {
            if (!vendor_entry.is_directory())
                continue;
            fs::path vendor_yaml_path = vendor_entry.path() / fs::path{"vendor.yaml"};

            if (!fs::exists(fs::directory_entry(vendor_yaml_path)))
                continue;

            try {
                Domain::Preset::VendorBundle vendor_bundle;

                config_loader.load(vendor_yaml_path.string());
                vendor_bundle.vendor_data = config_loader.release();

                preset_loader.load_dir(vendor_entry.path().string());
                vendor_bundle.presets = preset_loader.release();

                vendor_bundle.printer_configs = load_vendor_user_configs((config_base / vendor_bundle.vendor_data.info.id).string(), vendor_bundle.vendor_data);

                bundle.vendor_bundles.emplace(vendor_bundle.vendor_data.info.id, std::move(vendor_bundle));
            }
            catch (Yaml::ParseError& e) {
                SPDLOG_ERROR("Loading bundle {} failed with error {}", vendor_entry.path().string(), e.what());
            }
            catch (fs::filesystem_error& e) {
                SPDLOG_ERROR("Loading bundle {} failed with error {}", vendor_entry.path().string(), e.what());
            }
            catch (std::exception& e) {
                SPDLOG_ERROR("Loading bundle {} failed with error {}", vendor_entry.path().string(), e.what());
            }

        }

    }

    return bundle;
}

void save_bundle_configs(const Domain::Preset::Bundle& bundle, const std::string& config_path)
{
    fs::path config_base{config_path};
    for (const auto& [_, vendor_bundle] : bundle.vendor_bundles)
        save_vendor_user_configs(
            vendor_bundle.printer_configs,
            (config_base / vendor_bundle.vendor_data.info.id).string(),
            vendor_bundle.vendor_data
        );
}

}