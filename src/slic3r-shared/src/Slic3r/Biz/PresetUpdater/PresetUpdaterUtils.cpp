#include "PresetUpdaterUtils.hpp"
#include "PresetUpdaterIndex.hpp"
#include "PresetUpdaterProcessStatus.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterReconfigurationList.hpp"
#include "Slic3r/Biz/Utils/CopyFile.hpp"
#include "Slic3r/Biz/Directories.hpp"
#include "Slic3r/Biz/Preset/IO/HwConfigLoader.hpp"

#include "Slic3r/Exception.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/system/error_code.hpp>
#include <boost/nowide/fstream.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PresetUpdater {

bool copy_file_wrapper(const fs::path& source, const fs::path& target, PresetUpdaterProcessStatus* process_status)
{
    SPDLOG_INFO("PresetUpdater: Copying {} -> {}", source.string(), target.string());
    std::string error_message;
    Biz::Utils::CopyFileResult
        cfr = Biz::Utils::copy_file(source.string(), target.string(), error_message, false);
    if (cfr != Biz::Utils::CopyFileResult::Success) {
        std::string msg = fmt::format(
            "Copying of file {} to {} failed: {}",
            source.string(),
            target.string(),
            error_message
        );
        SPDLOG_ERROR(msg);
        process_status->set_error(msg);
        return false;
    }
    // Permissions should be copied from the source file by copy_file(). We are not sure about the source
    // permissions, let's rewrite them with 644.
    static constexpr const auto perms = fs::owner_read
        | fs::owner_write
        | fs::group_read
        | fs::others_read;
    fs::permissions(target, perms);
    return true;
}

/// Load all idx from single directory
std::vector<PresetUpdaterIndex> load_vendors_db(const fs::path& archive_path)
{
    std::vector<PresetUpdaterIndex> index_db;
    std::string errors_cumulative;
    boost::system::error_code ec;
    if (!fs::exists(archive_path, ec) || ec) {
        throw Slic3r::RuntimeError(archive_path.string() + " does not exists. " + ec.message());
    }
    if (!fs::is_directory(archive_path, ec) || ec) {
        throw Slic3r::RuntimeError(archive_path.string() + " is not directory. " + ec.message());
    }
    ec.clear();
    for (auto& dir_entry : fs::directory_iterator(archive_path, ec)) {
        if (is_idx_file(dir_entry)) {
            PresetUpdaterIndex idx;
            try {
                idx.load(dir_entry.path());
            } catch (const std::runtime_error& err) {
                errors_cumulative += err.what();
                errors_cumulative += "\n";
                continue;
            }
            if (std::find_if(
                    index_db.begin(),
                    index_db.end(),
                    [idx](const PresetUpdaterIndex& index) { return idx.vendor() == index.vendor(); }
                )
                == index_db.end())
                index_db.emplace_back(std::move(idx));
        }
    }
    if (!errors_cumulative.empty())
        throw Slic3r::RuntimeError(errors_cumulative);
    return index_db;
}

std::vector<PresetUpdaterIndex> load_vendors_db_filtered(
    const boost::filesystem::path& from_path,
    const std::vector<std::string>& filter
)
{
    // We want only indicies of vendors from filter vector
    // Rest of the idx files are vendors of different archive
    std::vector<PresetUpdaterIndex> index_db;
    std::string errors_cumulative;
    boost::system::error_code ec;
    if (!fs::exists(from_path, ec) || ec) {
        throw Slic3r::RuntimeError(from_path.string() + " does not exists. " + ec.message());
    }
    if (!fs::is_directory(from_path, ec) || ec) {
        throw Slic3r::RuntimeError(from_path.string() + " is not directory. " + ec.message());
    }

    fs::directory_iterator source_directory_iterator(from_path, ec);
    // PresetUpdaterIndex work with dir entries, so we iterate every time
    for (auto& dir_entry : source_directory_iterator) {
        if (is_idx_file(dir_entry)) {
            if (std::find(filter.begin(), filter.end(), dir_entry.path().filename().string())
                == filter.end())
            {
                // not an index from this archive
                continue;
            }
            PresetUpdaterIndex idx;
            try {
                idx.load(dir_entry.path());
            } catch (const std::runtime_error& err) {
                errors_cumulative += err.what();
                errors_cumulative += "\n";
                continue;
            }
            if (std::find_if(
                    index_db.begin(),
                    index_db.end(),
                    [idx](const PresetUpdaterIndex& index) { return idx.vendor() == index.vendor(); }
                )
                == index_db.end())
            {
                index_db.emplace_back(std::move(idx));
            } else {
                ASSERT(false);
            }
        }
    }
    if (!errors_cumulative.empty())
        throw Slic3r::RuntimeError(errors_cumulative);
    return index_db;
}

std::map<std::string, std::string> read_version_manifest(
    const fs::path& path,
    PresetUpdaterProcessStatus* process_status
)
{
    std::map<std::string, std::string> manifest_data;
    boost::nowide::ifstream file(path);

    if (!file.is_open()) {
        std::string msg = fmt::format("Failed to read version manifest {}", path.string());
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return {};
    }

    nlohmann::json data;
    try {
        // Parse the JSON directly from the file stream
        file >> data;
    } catch (nlohmann::json::parse_error& e) {
        std::string msg = fmt::format("Failed to read version manifest {}. {}", path.string(), e.what());
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return {};
    }

    if (!data.is_array()) {
        std::string msg = fmt::format(
            "Failed to read version manifest {}. Uexpected json formating.",
            path.string()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return {};
    }

    for (const auto& item : data) {
        // Ensure the element is an object and contains the required keys
        if (item.is_object()
            && item.contains("filename")
            && item.contains("filehash")
            && item["filename"].is_string()
            && item["filehash"].is_string())
        {
            std::string filename    = item.at("filename").get<std::string>();
            std::string filehash    = item.at("filehash").get<std::string>();
            manifest_data[filename] = filehash;
        } else {
            std::string msg = fmt::format(
                "Failed to read version manifest {}. Uexpected json formating.",
                path.string()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return {};
        }
    }
    return manifest_data;
}

namespace {

#if 0
void perform_downgrades(
    const std::vector<VendorReconfiguration>& downgrades,
    PresetUpdaterProcessStatus* process_status
)
{
    for (const VendorReconfiguration& downgrade : downgrades) {
        SPDLOG_INFO("Deleting incompatible bundle {}", downgrade.vendor_id);
        fs::path dir_path = fs::path(data_dir())
            / "profiles"
            / "local"
            / "vendor"
            / downgrade.vendor_repo_id
            / downgrade.vendor_id;
        fs::path idx_path = fs::path(data_dir())
            / "profiles"
            / "local"
            / "vendor"
            / downgrade.vendor_repo_id
            / (downgrade.vendor_id + ".idx");
        boost::system::error_code ec;
        if (!fs::remove_all(dir_path, ec) || ec) {
            process_status->set_error(
                fmt::format(
                    "Failed to remove directory {} while performing downgrade. {}",
                    dir_path.string(),
                    ec.message()
                )
            );
            return;
        }
        if (!fs::remove(idx_path, ec) || ec) {
            process_status->set_error(
                fmt::format(
                    "Failed to remove index {} while performing downgrade. {}",
                    idx_path.string(),
                    ec.message()
                )
            );
            return;
        }
    }
}
#endif // 0

void perform_update_no_source_manifest(
    const VendorReconfiguration& update,
    PresetUpdaterProcessStatus* process_status
)
{
    boost::system::error_code ec;

    const fs::path profile_local_path = fs::path(data_dir()) / "profiles" / "local" / "vendor";
    const fs::path update_sync_path   = fs::path(data_dir()) / "update_sync";

    const fs::path vendor_dest_dir_path = profile_local_path / update.vendor_repo_id / update.vendor_id;
    const fs::path vendor_dest_idx_path = profile_local_path
        / update.vendor_repo_id
        / (update.vendor_id + ".idx");

    const fs::path vendor_source_dir_path = update_sync_path / update.vendor_repo_id / update.vendor_id;
    const fs::path vendor_source_idx_path = update_sync_path
        / update.vendor_repo_id
        / (update.vendor_id + ".idx");
    const fs::path vendor_source_manifest_path = vendor_source_dir_path
        / (update.vendor_id + ".manifest");

    ASSERT(fs::exists(vendor_source_dir_path) && fs::is_directory(vendor_source_dir_path));
    ASSERT(fs::exists(vendor_source_idx_path) && fs::is_regular_file(vendor_source_idx_path));

    ec.clear();
    if (!fs::create_directories(vendor_dest_dir_path, ec) && ec) {
        process_status->set_error(
            fmt::format("Failed to create directory {}: {}", vendor_dest_dir_path.string(), ec.message())
        );
        return;
    }

    // Delete all files in source
    for (const auto& entry : fs::directory_iterator(vendor_dest_dir_path, ec)) {
        const fs::path& path = entry.path();
        SPDLOG_INFO("Deleting {}", path.string());
        ec.clear();
        if (!fs::remove_all(path, ec) || ec) {
            process_status->set_error(
                fmt::format("Failed to remove file {}: {}", path.string(), ec.message())
            );
            return;
        }
    }
    if (ec) {
        std::string msg = fmt::format(
            "{}: Error traversing directory {}: {}",
            std::string(__FUNCTION__),
            vendor_dest_dir_path.string(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // move alle files from source
    for (const auto& entry : fs::recursive_directory_iterator(vendor_source_dir_path, ec)) {
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }
        const fs::path& source_path = entry.path();
        fs::path relative_path      = fs::relative(source_path, vendor_source_dir_path);
        fs::path dest_path          = vendor_dest_dir_path / relative_path;

        if (!fs::create_directories(dest_path.parent_path(), ec) && ec) {
            process_status->set_error(
                fmt::format(
                    "Failed to create directory {}: {}",
                    dest_path.parent_path().string(),
                    ec.message()
                )
            );
            return;
        }

        fs::rename(source_path, dest_path, ec);
        if (ec) {
            process_status->set_error(
                fmt::format(
                    "Failed to move file {} to {}: {}",
                    source_path.string(),
                    dest_path.string(),
                    ec.message()
                )
            );
            return;
        }
    }
    if (ec) {
        std::string msg = fmt::format(
            "{}: Error traversing directory {}: {}",
            std::string(__FUNCTION__),
            vendor_source_dir_path.string(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // move index
    ec.clear();
    fs::rename(vendor_source_idx_path, vendor_dest_idx_path, ec);
    if (ec) {
        process_status->set_error(
            fmt::format(
                "Failed to move file {} to {}: {}",
                vendor_source_idx_path.string(),
                vendor_dest_idx_path.string(),
                ec.message()
            )
        );
        return;
    }

    // cleanup source
    ec.clear();
    if (!fs::remove_all(vendor_source_dir_path, ec) || ec) {
        process_status->set_error(
            fmt::format("Failed to remove {}: {}", vendor_source_dir_path.string(), ec.message())
        );
        return;
    }
}

void perform_update_with_source_manifest(
    const VendorReconfiguration& update,
    PresetUpdaterProcessStatus* process_status
)
{
    boost::system::error_code ec;

    const fs::path profile_local_path = fs::path(data_dir()) / "profiles" / "local" / "vendor";
    const fs::path update_sync_path   = fs::path(data_dir()) / "update_sync";

    const fs::path vendor_dest_dir_path = profile_local_path / update.vendor_repo_id / update.vendor_id;
    const fs::path vendor_dest_idx_path = profile_local_path
        / update.vendor_repo_id
        / (update.vendor_id + ".idx");

    const fs::path vendor_source_dir_path = update_sync_path / update.vendor_repo_id / update.vendor_id;
    const fs::path vendor_source_idx_path = update_sync_path
        / update.vendor_repo_id
        / (update.vendor_id + ".idx");
    const fs::path vendor_source_manifest_path = vendor_source_dir_path
        / (update.vendor_id + ".manifest");

    ASSERT(fs::exists(vendor_source_dir_path) && fs::is_directory(vendor_source_dir_path));
    ASSERT(fs::exists(vendor_source_idx_path) && fs::is_regular_file(vendor_source_idx_path));
    ASSERT(
        fs::exists(vendor_source_manifest_path) && fs::is_regular_file(vendor_source_manifest_path, ec)
    );

    ec.clear();
    if (!fs::create_directory(vendor_dest_dir_path, ec) && ec) {
        process_status->set_error(
            fmt::format("Failed to create directory {}: {}", vendor_dest_dir_path.string(), ec.message())
        );
        return;
    }

    // Read manifest
    std::map<std::string, std::string> files_in_version_manifest = read_version_manifest(
        vendor_source_manifest_path,
        process_status
    );

    // Delete files that are not in manifest
    for (const auto& entry : fs::recursive_directory_iterator(vendor_dest_dir_path, ec)) {
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }
        const fs::path& path = entry.path();
        if (files_in_version_manifest.find(path.filename().string())
            == files_in_version_manifest.end())
        {
            if (!fs::remove(path, ec) || ec) {
                process_status->set_error(
                    fmt::format("Failed to remove file {}: {}", path.string(), ec.message())
                );
                return;
            }
        }
    }
    if (ec) {
        std::string msg = fmt::format(
            "{}: Error traversing directory {}: {}",
            std::string(__FUNCTION__),
            vendor_dest_dir_path.string(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // move files that are staged
    for (const auto& entry : fs::recursive_directory_iterator(vendor_source_dir_path, ec)) {
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }
        const fs::path& source_path = entry.path();
        fs::path relative_path      = fs::relative(source_path, vendor_source_dir_path);
        fs::path dest_path          = vendor_dest_dir_path / relative_path;
        // create subdir
        if (!fs::create_directories(dest_path.parent_path(), ec) && ec) {
            std::string msg = fmt::format(
                "Failed to create target directory {}: {}. Staging update has failed.",
                dest_path.parent_path().string(),
                ec.message()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        // move file
        fs::rename(source_path, dest_path, ec);
        if (ec) {
            process_status->set_error(
                fmt::format(
                    "Failed to move file {} to {}: {}",
                    source_path.string(),
                    dest_path.string(),
                    ec.message()
                )
            );
            return;
        }
    }
    // move index
    ec.clear();
    fs::rename(vendor_source_idx_path, vendor_dest_idx_path, ec);
    if (ec) {
        process_status->set_error(
            fmt::format(
                "Failed to move file {} to {}: {}",
                vendor_source_idx_path.string(),
                vendor_dest_idx_path.string(),
                ec.message()
            )
        );
        return;
    }

    // cleanup source
    ec.clear();
    if (!fs::remove_all(vendor_source_dir_path, ec) || ec) {
        process_status->set_error(
            fmt::format("Failed to remove {}: {}", vendor_source_dir_path.string(), ec.message())
        );
        return;
    }
}

void perform_updates(
    const std::vector<VendorReconfiguration>& updates,
    PresetUpdaterProcessStatus* process_status
)
{
    const fs::path update_sync_path = fs::path(data_dir()) / "update_sync";
    boost::system::error_code ec;
    for (const VendorReconfiguration& update : updates) {
        SPDLOG_INFO("Updating bundle {}", update.vendor_id);

        const fs::path vendor_source_manifest_path = update_sync_path
            / update.vendor_repo_id
            / update.vendor_id
            / (update.vendor_id + ".manifest");
        ec.clear();
        if (fs::exists(vendor_source_manifest_path, ec)
            && !ec
            && fs::is_regular_file(vendor_source_manifest_path, ec)
            && !ec)
        {
            perform_update_with_source_manifest(update, process_status);
        } else {
            perform_update_no_source_manifest(update, process_status);
        }
    }
}
} // namespace

void check_forced_reconfigurations(
    PresetUpdaterReconfigurationList& results,
    PresetUpdaterProcessStatus* process_status
)
{
    const fs::path profile_local_path = fs::path(data_dir()) / "profiles" / "local" / "vendor";
    boost::system::error_code ec;

    if (!fs::exists(profile_local_path, ec) || ec) {
        process_status->set_error(profile_local_path.string() + " does not exists. " + ec.message());
        return;
    }
    if (!fs::is_directory(profile_local_path, ec) || ec) {
        process_status->set_error(profile_local_path.string() + " is not directory. " + ec.message());
        return;
    }

    // vendors are inside repo_id dir
    ec.clear();
    for (auto& archive_dir : fs::directory_iterator(profile_local_path, ec)) {
        if (process_status->get_canceled()) {
            return;
        }
        ec.clear();
        if (!fs::is_directory(archive_dir.path(), ec) || ec) {
            continue;
        }
        const std::string repo_id = archive_dir.path().filename().string();

        std::vector<PresetUpdaterIndex> index_db;
        try {
            index_db = load_vendors_db(archive_dir.path());
        } catch (const std::exception& e) {
            std::string msg = fmt::format("Loading index db of {} has failed {}", repo_id, e.what());
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        for (const auto& index : index_db) {
            const fs::path installed_vendor_yaml_path = archive_dir.path()
                / index.vendor()
                / "vendor.yaml";
            ec.clear();

            if (!fs::exists(installed_vendor_yaml_path, ec) || ec) {
                process_status->set_warning(
                    fmt::format(
                        "File does not exists {}. {}",
                        installed_vendor_yaml_path.string(),
                        ec.message()
                    )
                );
                continue;
            }

            // Current version
            Preset::IO::HwConfigLoader hw_config_loader;
            Domain::Preset::VendorData vendor_data;

            try {
                vendor_data = hw_config_loader.load(installed_vendor_yaml_path.string());
            } catch (const std::exception& e) {
                process_status->set_error(
                    fmt::format(
                        "Failed to load vendor file {}: {}",
                        installed_vendor_yaml_path.string(),
                        e.what()
                    )
                );
                continue;
            }
            Semver current_version = Semver(vendor_data.info.version);

            // Recommended version of vendor
            const PresetUpdaterIndex::const_iterator recommended = index.recommended();
            if (recommended == index.end()) {
                SPDLOG_ERROR(
                    "No recommended version for vendor: {}, Index file might be corrupted.",
                    index.vendor()
                );
                DEBUG_ASSERT(false);
                continue;
            }
            const PresetUpdaterIndex::const_iterator vendor_current_version_it = index.find(
                current_version
            );
            const bool ver_current_found = vendor_current_version_it != index.end();

            SPDLOG_INFO(
                "Vendor: {}, version installed: {}{}, recommended version: {}",
                vendor_data.info.name,
                current_version.to_string(),
                (ver_current_found ? "" : " (not found in index!)"),
                recommended->config_version.to_string()
            );

            if (!ver_current_found) {
                // Any published config shall be always found in the latest config index.
                SPDLOG_ERROR(
                    "Preset bundle `{}` version not found in index: {}",
                    index.vendor(),
                    current_version.to_string()
                );
                if (process_status->has_warning(repo_id, vendor_data.info.id)) {
                    SPDLOG_ERROR(
                        "Vendor {} of {} repository is not added to reconfigurations due to previous warnings.",
                        vendor_data.info.id,
                        repo_id
                    );
                    continue;
                }
                results.emplace_back(
                    VendorReconfigurationState::NotInIndex,
                    vendor_data.info.id,
                    repo_id,
                    Slic3r::Semver(),
                    recommended->config_version,
                    recommended->comment,
                    std::string(),
                    std::string()
                );
                continue;
            }

            if (vendor_current_version_it->is_current_slic3r_supported()) {
                // This slicer needs no forced reconfigurations
                SPDLOG_ERROR(
                    "Preset bundle `{}` version {} needs no forced reconfiguration.",
                    index.vendor(),
                    current_version.to_string()
                );
                continue;
            }

            if (vendor_current_version_it->is_current_slic3r_downgrade()) {
                SPDLOG_WARN(
                    "Current Slic3r incompatible with installed bundle (forced downgrade): {}",
                    installed_vendor_yaml_path.string()
                );
                if (process_status->has_warning(repo_id, vendor_data.info.id)) {
                    SPDLOG_ERROR(
                        "Vendor {} of {} repository is not added to reconfigurations due to previous warnings.",
                        vendor_data.info.id,
                        repo_id
                    );
                    continue;
                }
                results.emplace_back(
                    VendorReconfigurationState::ForcedDowngrade,
                    vendor_data.info.id,
                    repo_id,
                    vendor_current_version_it->config_version,
                    recommended->config_version,
                    recommended->comment,
                    std::string(),
                    std::string()
                );
                continue;
            }
            SPDLOG_WARN(
                "Current Slic3r incompatible with installed bundle (forced update): {}",
                installed_vendor_yaml_path.string()
            );
            if (process_status->has_warning(repo_id, vendor_data.info.id)) {
                SPDLOG_ERROR(
                    "Vendor {} of {} repository is not added to reconfigurations due to previous warnings.",
                    vendor_data.info.id,
                    repo_id
                );
                continue;
            }
            results.emplace_back(
                VendorReconfigurationState::ForcedUpdate,
                vendor_data.info.id,
                repo_id,
                vendor_current_version_it->config_version,
                recommended->config_version,
                recommended->comment,
                std::string(),
                std::string()
            );
        }
    }
}

void check_reconfigurations(
    PresetUpdaterReconfigurationList& results,
    PresetUpdaterProcessStatus* process_status
)
{
    // SPDLOG_INFO(__FUNCTION__);

    const fs::path profile_local_path = fs::path(data_dir()) / "profiles" / "local" / "vendor";
    const fs::path update_sync_path   = fs::path(data_dir()) / "update_sync";
    boost::system::error_code ec;

    if (!fs::exists(profile_local_path, ec) || ec) {
        process_status->set_error(profile_local_path.string() + " does not exists. " + ec.message());
        return;
    }
    if (!fs::is_directory(profile_local_path, ec) || ec) {
        process_status->set_error(profile_local_path.string() + " is not directory. " + ec.message());
        return;
    }
    if (!fs::exists(update_sync_path, ec) || ec) {
        process_status->set_error(update_sync_path.string() + " does not exists. " + ec.message());
        return;
    }
    if (!fs::is_directory(update_sync_path, ec) || ec) {
        process_status->set_error(update_sync_path.string() + " is not directory. " + ec.message());
        return;
    }

    // vendors are inside repo_id dir
    for (auto& archive_dir : fs::directory_iterator(profile_local_path, ec)) {
        if (process_status->get_canceled()) {
            return;
        }
        ec.clear();
        if (!fs::is_directory(archive_dir.path(), ec) || ec) {
            continue;
        }
        const std::string repo_id = archive_dir.path().filename().string();

        std::vector<PresetUpdaterIndex> index_db;
        try {
            index_db = load_vendors_db(archive_dir.path());
        } catch (const std::exception& e) {
            std::string msg = fmt::format("Loading index db of {} has failed {}", repo_id, e.what());
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        for (const auto& index : index_db) {
            // the installed bundles are in / "profiles" / "local"
            // the most recent index file should be in / "update_sync"
            // if index in / "profiles" / "local" is more recent use it instead
            // (should the index  in / "profiles" / "local" be rewritten if not most recent?)
            // then check installed bundles version and decide if and what type of reconfiguration

            // Compare update_sync index and index
            const fs::path update_sync_index_path = update_sync_path
                / archive_dir.path().filename()
                / index.path().filename();
            PresetUpdaterIndex most_recent_index;
            ec.clear();
            if (fs::exists(update_sync_index_path, ec) && !ec) {
                PresetUpdaterIndex update_sync_index;
                try {
                    update_sync_index.load(update_sync_index_path);
                } catch (const std::runtime_error& err) {
                    process_status->set_warning(
                        "Failed to load index " + update_sync_index_path.string()
                    );
                    continue;
                }
                if (update_sync_index.version() > index.version()) {
                    most_recent_index = std::move(update_sync_index);
                } else {
                    most_recent_index = index;
                }
            } else {
                most_recent_index = index;
            }
            // use most_recent_index from now
            const fs::path installed_vendor_yaml_path = archive_dir.path()
                / index.vendor()
                / "vendor.yaml";

            // Current version
            Preset::IO::HwConfigLoader hw_config_loader;
            Domain::Preset::VendorData vendor_data;
            try {
                vendor_data = hw_config_loader.load(installed_vendor_yaml_path.string());
            } catch (const std::exception& e) {
                process_status->set_error(
                    fmt::format(
                        "Failed to load vendor file {}: {}",
                        installed_vendor_yaml_path.string(),
                        e.what()
                    )
                );
                continue;
            }
            Semver current_version = Semver(vendor_data.info.version);

            // Recommended version of vendor
            const PresetUpdaterIndex::const_iterator recommended = most_recent_index.recommended();
            if (recommended == most_recent_index.end()) {
                std::string msg = fmt::format(
                    "No recommended version for vendor: {}, Index file might be corrupted.",
                    index.vendor()
                );
                SPDLOG_ERROR(msg);
                process_status->set_warning(msg);
                DEBUG_ASSERT(false);
                continue;
            }
            const PresetUpdaterIndex::const_iterator vendor_current_version_it = most_recent_index.find(
                current_version
            );
            const bool current_found = vendor_current_version_it != most_recent_index.end();
            SPDLOG_INFO(
                "Vendor: {}, version installed: {}{}, recommended version: {}",
                vendor_data.info.name,
                current_version.to_string(),
                (current_found ? "" : " (not found in index!)"),
                recommended->config_version.to_string()
            );
            if (!current_found) {
                SPDLOG_WARN(
                    "Current installed version for vendor: {} ({}) was not found in index %3%",
                    vendor_data.info.id,
                    current_version.to_string(),
                    most_recent_index.path().string()
                );
                // If current is not found in index, we need forced reconfiguration
                if (process_status->has_warning(repo_id, vendor_data.info.id)) {
                    SPDLOG_ERROR(
                        "Vendor {} of {} repository is not added to reconfigurations due to previous warnings.",
                        vendor_data.info.id,
                        repo_id
                    );
                    continue;
                }
                results.emplace_back(
                    VendorReconfigurationState::NotInIndex,
                    vendor_data.info.id,
                    repo_id,
                    Slic3r::Semver(),
                    recommended->config_version,
                    recommended->comment,
                    std::string(),
                    std::string()
                );
                continue;
            }
            if (vendor_current_version_it->is_current_slic3r_supported()) {
                // This slicer needs no forced reconfigurations
                ASSERT(current_version <= recommended->config_version);
                if (current_version < recommended->config_version) {
                    // regular update
                    SPDLOG_INFO(
                        "Preset bundle `{}` version {} has update {}.",
                        index.vendor(),
                        current_version.to_string(),
                        recommended->config_version.to_string()
                    );
                    if (process_status->has_warning(repo_id, vendor_data.info.id)) {
                        SPDLOG_ERROR(
                            "Vendor {} of {} repository is not added to reconfigurations due to previous warnings.",
                            vendor_data.info.id,
                            repo_id
                        );
                        continue;
                    }
                    results.emplace_back(
                        VendorReconfigurationState::Update,
                        vendor_data.info.id,
                        repo_id,
                        vendor_current_version_it->config_version,
                        recommended->config_version,
                        recommended->comment,
                        std::string(),
                        std::string()
                    );
                }
                continue;
            }

            if (vendor_current_version_it->is_current_slic3r_downgrade()) {
                // Forced downgrade
                SPDLOG_WARN(
                    "Current Slic3r incompatible with installed bundle (forced downgrade): {}",
                    installed_vendor_yaml_path.string()
                );
                if (process_status->has_warning(repo_id, vendor_data.info.id)) {
                    SPDLOG_ERROR(
                        "Vendor {} of {} repository is not added to reconfigurations due to previous warnings.",
                        vendor_data.info.id,
                        repo_id
                    );
                    continue;
                }
                results.emplace_back(
                    VendorReconfigurationState::ForcedDowngrade,
                    vendor_data.info.id,
                    repo_id,
                    vendor_current_version_it->config_version,
                    recommended->config_version,
                    recommended->comment,
                    std::string(),
                    std::string()
                );
                continue;
            }
            // Not supported and not downgrade = forced update
            SPDLOG_WARN(
                "Current Slic3r incompatible with installed bundle (forced update): {}",
                installed_vendor_yaml_path.string()
            );
            if (process_status->has_warning(repo_id, vendor_data.info.id)) {
                SPDLOG_ERROR(
                    "Vendor {} of {} repository is not added to reconfigurations due to previous warnings.",
                    vendor_data.info.id,
                    repo_id
                );
                continue;
            }
            results.emplace_back(
                VendorReconfigurationState::ForcedUpdate,
                vendor_data.info.id,
                repo_id,
                vendor_current_version_it->config_version.to_string(),
                recommended->config_version.to_string(),
                recommended->comment,
                std::string(),
                std::string()
            );
        }
    }
}

void perform_reconfigurations(
    const PresetUpdaterReconfigurationList& reconfigurations,
    PresetUpdaterProcessStatus* process_status
)
{
    if (!reconfigurations.forced_downgrades().empty()) {
        // In case of existing downgrade, we only perform downgrade.
        // The rest will be done in following wizard.
        perform_updates(reconfigurations.forced_downgrades(), process_status);
        return;
    }
    // Forced and regular updates are now the same.
    // The difference was if this functions "must" or "should" be called.
    perform_updates(reconfigurations.forced_updates(), process_status);
    perform_updates(reconfigurations.regular_updates(), process_status);
}

} // namespace Slic3r::Biz::PresetUpdater
