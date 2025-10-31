#include "Slic3r/Biz/Preset/IO/BundleLoader.hpp"

#include "Slic3r/Log.hpp"
#include "Slic3r/Biz/Yaml/Yaml.hpp"
#include "Slic3r/Biz/Preset/IO/PresetLoader.hpp"
#include "Slic3r/Biz/Preset/IO/HwConfigLoader.hpp"
#include "Slic3r/Biz/Algorithms/CerealUtils.hpp"

#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/nowide/fstream.hpp>

#include <functional>
#include <numeric>

#include <cereal/archives/binary.hpp>

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
                vendor_bundle.vendor_data.info.repo_id = repo_entry.path().filename().string();

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


static size_t combine_hashes(size_t hash1, size_t hash2)
{
    return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
}

static size_t get_file_hash(const std::string& path)
{
    boost::nowide::ifstream file(path, std::ios::binary);
    if (! file)
        throw std::runtime_error(std::string("Unable to get hash for ") + path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return std::hash<std::string>{}(content);
}

static size_t hash_folder_recursive(const fs::path& path)
{
    size_t seed = 0;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (fs::is_regular_file(entry))
                seed ^= combine_hashes(get_file_hash(entry.path().string()), seed);
        }
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error(std::string("Error iterating directory ") + path.string() + ": " + e.what());
    }
    return seed;
}

static size_t get_cache_footprint(const std::string& preset_bundle_path, const std::string& config_path, const std::string& slicer_version)
{
    size_t hash1 = 0;
    boost::system::error_code ec;
    if (fs::exists(preset_bundle_path, ec))
        hash1 = hash_folder_recursive(preset_bundle_path);
    size_t hash2 = 0;
    if (fs::exists(config_path, ec))
        hash2 = hash_folder_recursive(config_path);
    // Combine the two hashes and a hash of slicer version
    size_t hash = combine_hashes(combine_hashes(hash1, hash2), std::hash<std::string>{}(slicer_version));

    // Increment the following value to enforce invalidation of caches from older versions:
    size_t cache_epoch = 3;
    return combine_hashes(hash, std::hash<int>{}(cache_epoch));
}

void serialize_bundle(const std::string& filename, const Domain::Preset::Bundle& bundle,
    const std::string& preset_bundle_path, const std::string& config_path, const std::string& slicer_version)
{
    SPDLOG_DEBUG("Saving currently loaded bundle into cache file...");
    try {
        if (const auto dir = fs::path(filename).parent_path(); ! boost::filesystem::exists(dir))
            boost::filesystem::create_directory(dir);
    } catch (const boost::filesystem::filesystem_error&) {
        SPDLOG_ERROR("Unable to create bundle cache directory.");
    }
    boost::nowide::ofstream os(filename, std::ios::binary);
    if (os) {
        try {
            os << get_cache_footprint(preset_bundle_path, config_path, slicer_version);
        } catch (const std::runtime_error&) {
            SPDLOG_ERROR("Unable to calculate current bundle hash.");
        }
        cereal::BinaryOutputArchive archive(os);
        archive(bundle);
        SPDLOG_DEBUG("Loaded bundle cached sucessfully.");
    } else
        SPDLOG_ERROR("Unable to create bundle cache file.");
}

std::optional<Domain::Preset::Bundle> deserialize_bundle(const std::string& filename,
    const std::string& preset_bundle_path, const std::string& config_path, const std::string& slicer_version)
{
    SPDLOG_DEBUG("Running in debug mode - will try to recover bundle from cache...");
    if (boost::nowide::ifstream is(filename, std::ios::binary); is) {
        SPDLOG_DEBUG("Bundle cache file found: {}", filename);
        size_t cache_hash = 0;
        is >> cache_hash;
        size_t cur_hash = 0;
        try {
            cur_hash = get_cache_footprint(preset_bundle_path, config_path, slicer_version);
        } catch (const std::runtime_error&) {
            SPDLOG_ERROR("Unable to calculate current bundle hash.");
            return std::nullopt;
        }
        if (cache_hash != cur_hash) {
            SPDLOG_DEBUG("Bundle hashes do NOT match.");
            return std::nullopt;
        }            
        SPDLOG_DEBUG("Bundle hashes match. Deserializing bundle from cache...");
        try {
            cereal::BinaryInputArchive archive(is);
            Domain::Preset::Bundle bundle;
            archive(bundle);
            SPDLOG_DEBUG("Sucessfully recovered bundle from cache.");
            return std::make_optional(std::move(bundle));
        } catch (const std::exception& ex) {
            SPDLOG_ERROR("Unable to deserialize bundle from cache: {}", ex.what());
        }        
    } else
        SPDLOG_DEBUG("Bundle cache file not found.");
    return std::nullopt;
}

}
