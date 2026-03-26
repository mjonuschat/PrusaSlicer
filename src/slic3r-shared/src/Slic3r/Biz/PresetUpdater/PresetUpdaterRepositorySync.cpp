#include "PresetUpdaterRepositorySync.hpp"

#include "PresetUpdaterProcessStatus.hpp"
#include "PresetUpdaterRepository.hpp"
#include "PresetUpdaterIndex.hpp"
#include "PresetUpdaterFileHash.hpp"
#include "PresetUpdaterUtils.hpp"
#include "Slic3r/Directories.hpp"
#include "Slic3r/Biz/Preset/IO/HwConfigLoader.hpp"

#include "Slic3r/Exception.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Directories.hpp"

#include "libslic3r/miniz_extension.hpp"

#include <fmt/format.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <expected>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PresetUpdater {

namespace {
fs::path create_temp_dir()
{
    boost::uuids::random_generator generator;
    const std::string dirname = boost::uuids::to_string(generator());
    fs::path temp_dir         = Slic3r::temp_dir() / dirname;

    boost::system::error_code ec;

    if (fs::exists(temp_dir, ec) && !ec && fs::is_directory(temp_dir, ec) && !ec) {
        SPDLOG_ERROR(
            "Temp directory {} already exists.",
            temp_dir.string()
        ); // Something is off. This should never happen.
        throw Slic3r::RuntimeError(
            "Failed to create temp directory " + temp_dir.string() + ". " + ec.message()
        );
    }

    if (!fs::create_directory(temp_dir, ec)) {
        throw Slic3r::RuntimeError(
            "Failed to create temp directory " + temp_dir.string() + ". " + ec.message()
        );
    }
    return temp_dir;
}

bool is_vendor_installed(const std::string& vendor_id, const std::string& repo_id)
{
    const fs::path vendor_folder_path    = local_vendor_path(repo_id, vendor_id);
    boost::system::error_code ec;

    if (!fs::exists(vendor_folder_path, ec) || ec) {
        return false;
    }
    if (!fs::is_directory(vendor_folder_path, ec) || ec) {
        return false;
    }
    // lets take presence of vendor.yaml file as proof of installation
    fs::path vendor_index_path = vendor_folder_path / "vendor.yaml";
    if (!fs::exists(vendor_index_path, ec) || ec) {
        return false;
    }
    return true;
}

bool check_resouces_vendor_sanity(
    const boost::filesystem::path& source_dir,
    const AbstractPresetUpdaterRepository* repo,
    PresetUpdaterProcessStatus* process_status,
    const PresetUpdaterIndex& source_index
)
{
    const fs::path source_vendor_dir  = source_dir / source_index.vendor();
    const fs::path source_vendor_yaml = source_vendor_dir / "vendor.yaml";

    const PresetUpdaterIndex::const_iterator recommended = source_index.recommended();
    PresetUpdaterIndex update_sync_index;
    if (recommended == source_index.end()) {
        process_status->set_error(
            fmt::format(
                "No recommended version for vendor: {}, Index file might be corrupted.",
                source_index.vendor()
            )
        );
        return false;
    }

    Domain::Preset::VendorData source_vendor_data;
    try {
        Preset::IO::HwConfigLoader hw_config_loader;
        source_vendor_data = hw_config_loader.load(source_vendor_yaml.string());
        Semver version = Semver(source_vendor_data.info.version);
        if (version != recommended->config_version) {
            process_status->set_error(
                fmt::format("Vendor data in resources failed sanity check. Its version is not recommended by its index. Vendor: {}. This installation of app is probably corrupted.", source_index.vendor())
            );
            return false;
        }
    } catch (const std::exception& e) {
        process_status->set_error(
            fmt::format("Failed to load vendor file {}: {}. This installation of app is probably corrupted.", source_vendor_yaml.string(), e.what())
        );
        return false;
    }

    return true;
}




} // namespace

void PresetUpdaterRepositorySync::sync(
    const SharedRepositoryVector& repositories,
    PresetUpdaterProcessStatus* process_status
) const
{
    ASSERT(process_status);

    // Create workspace in OS temp folder.
    fs::path temp_dir;
    const fs::path resources_dir = fs::path(Slic3r::resources_dir()) / "presets";

    try {
        temp_dir = create_temp_dir();
    } catch (const Slic3r::RuntimeError& e) {
        process_status->set_error(std::string("Preset Archive Sync has failed. ") + e.what());
        return;
    }

    for (const AbstractPresetUpdaterRepository* repo : repositories) {
        if (process_status->get_canceled()) {
            return;
        }
        stage_rencofigurations_from_resources(resources_dir / repo->descriptor().id, repo, process_status);
    }

    // perform sync on every repository
    for (const AbstractPresetUpdaterRepository* repo : repositories) {
        if (process_status->get_canceled()) {
            break;
        }
        this->sync_repository(temp_dir / repo->descriptor().id, repo, process_status);

        // remove empty directories
        const fs::path update_sync_repo_dir = fs::path(data_dir()) / "update_sync" / repo->descriptor().id;
        boost::system::error_code ec;
        if (fs::is_directory(update_sync_repo_dir, ec)) {
            if (!fs::remove(update_sync_repo_dir, ec) && ec) {
                if (ec != boost::system::errc::directory_not_empty) {
                    process_status->set_warning(
                        "Failed to remove empty repo dir " + update_sync_repo_dir.string() + ". " + ec.message()
                    );
                    return;
                }
            }
        }
    }

    boost::system::error_code ec;
    if (!fs::remove_all(temp_dir, ec) || ec) {
        std::string msg = fmt::format(
            "{}: Failed to delete directory {}.",
            std::string(__FUNCTION__),
            temp_dir.string()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

}

void PresetUpdaterRepositorySync::stage_rencofigurations_from_resources(
    const boost::filesystem::path& source_dir,
    const AbstractPresetUpdaterRepository* repo,
    PresetUpdaterProcessStatus* process_status
) const
{
    boost::system::error_code ec;
    // Each repo has its own subdir. It does not have to be in resouces.
    if (!fs::exists(source_dir) || ec) {
        SPDLOG_INFO(
            "{} Directory does not exists {}: {}. Skipping.",
            std::string(__FUNCTION__),
            source_dir.string(),
            ec.message()
        );
        return;
    }

    const fs::path update_sync_repo_dir = fs::path(data_dir()) / "update_sync" / repo->descriptor().id;
    if (!fs::create_directory(update_sync_repo_dir, ec) && ec) {
        process_status->set_error(
            "Failed to create repo dir " + update_sync_repo_dir.string() + ". " + ec.message()
        );
        return;
    }

    std::vector<PresetUpdaterIndex> index_db;
    try {
        index_db = load_vendors_db(source_dir);
    } catch (const std::exception& e) {
        std::string msg = fmt::format(
            "Loading index db of {} has failed {}",
            source_dir.string(),
            e.what()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return;
    }

    for (const PresetUpdaterIndex& source_index : index_db) {
        process_status->set_warning_target(repo->descriptor().id, source_index.vendor());
        if (process_status->get_canceled()) {
            return;
        }
        if (!check_resouces_vendor_sanity(source_dir, repo, process_status, source_index)) {
            continue;
        }
        if (!is_vendor_installed(source_index.vendor(), repo->descriptor().id)) {
            stage_not_installed_vendor_from_resources(source_dir, repo, process_status, source_index);
            continue;
        }
        stage_installed_vendor_from_resources(source_dir, repo, process_status, source_index);
    }
    process_status->clear_warning_target();
}

void PresetUpdaterRepositorySync::stage_not_installed_vendor_from_resources(
    const boost::filesystem::path& source_dir,
    const AbstractPresetUpdaterRepository* repo,
    PresetUpdaterProcessStatus* process_status,
    const PresetUpdaterIndex& source_index
) const
{
    const fs::path update_sync_vendor_dir = fs::path(data_dir())
        / "update_sync"
        / repo->descriptor().id
        / source_index.vendor();
    const fs::path update_sync_index_path = fs::path(data_dir())
        / "update_sync"
        / repo->descriptor().id
        / (source_index.vendor() + ".idx"); // index is outside vendor folder.
    const fs::path update_sync_vendor_yaml = update_sync_vendor_dir / "vendor.yaml";

    const fs::path source_vendor_dir  = source_dir / source_index.vendor();
    const fs::path source_vendor_yaml = source_vendor_dir / "vendor.yaml";

    boost::system::error_code ec;

    ASSERT(fs::exists(source_index.path()));
    ASSERT(fs::exists(source_vendor_dir) && fs::is_directory(source_vendor_dir));
    ASSERT(fs::exists(source_vendor_yaml) && fs::is_regular_file(source_vendor_yaml));

    // Recommended version of vendor
    const PresetUpdaterIndex::const_iterator recommended = source_index.recommended();
    PresetUpdaterIndex update_sync_index; // recommneded is an iterator - once its index object stops existing it ivalidates.
    if (recommended == source_index.end()) {
        process_status->set_error(
            fmt::format(
                "No recommended version for vendor: {}, Index file might be corrupted.",
                source_index.vendor()
            )
        );
        return;
    }

    // Check version staged in update_sync
    if (fs::exists(update_sync_index_path, ec) && !ec && fs::exists(update_sync_vendor_yaml, ec) && !ec) {
        bool loaded_update_sync_index = false;
        try {
            update_sync_index.load(update_sync_index_path);
            loaded_update_sync_index = true;
        } catch (const std::runtime_error&) {
            process_status->set_warning(
                "Failed to load index " + update_sync_index_path.string()
            );
        }
        if (loaded_update_sync_index) {
            const PresetUpdaterIndex::const_iterator update_sync_recommended = update_sync_index.recommended();
            if (update_sync_recommended->config_version > recommended->config_version) {
                // Update sync has some more recent data - nothing to do here
                return;

            }
            // read version of staged
            Domain::Preset::VendorData update_sync_vendor_data;
            try {
                Preset::IO::HwConfigLoader hw_config_loader;
                update_sync_vendor_data = hw_config_loader.load(update_sync_vendor_yaml.string());
                Semver update_sync_version = Semver(update_sync_vendor_data.info.version);
                if (update_sync_version == recommended->config_version) {
                    // staged version is recommended - nothing to do here
                    return;
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR(
                    "Failed to load vendor file {}: {}",
                    update_sync_vendor_yaml.string(),
                    e.what()
                );
            }
        }
    }

    // Now we know that data in update_sync are not the recommended version
    // We also know that data in source (resources) are usable data - they came with the installation of this binary.
    // We simply delete data in update_sync and replace them with source.
    if (!fs::remove(update_sync_index_path, ec) && ec) {
        process_status->set_warning(
            fmt::format(
                "Failed to remove file {}: {}",
                update_sync_index_path.string(),
                ec.message()
            )
        );
    }
    if (!fs::remove_all(update_sync_vendor_dir, ec) && ec) {
        process_status->set_warning(
            fmt::format(
                "Failed to remove file {}: {}",
                update_sync_vendor_dir.string(),
                ec.message()
            )
        );
    }
   

    // Perform check of


    // Copy source to update_sync

    if (!fs::create_directory(update_sync_vendor_dir, ec) && ec) {
        process_status->set_error(
            "Failed to create vendor dir " + update_sync_vendor_dir.string() + ". " + ec.message()
        );
        return;
    }

    if (!copy_file_wrapper(source_index.path(), update_sync_index_path, process_status)) {
        return;
    }
    for (const auto& entry : fs::recursive_directory_iterator(source_vendor_dir, ec)) {
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }
        const fs::path source  = entry.path();
        fs::path relative_path = fs::relative(source, source_vendor_dir);
        const fs::path target  = update_sync_vendor_dir / relative_path;
        if (!fs::create_directories(target.parent_path(), ec) && ec) {
            std::string msg = fmt::format(
                "Failed to create target directory {}: {}. Staging update has failed.",
                target.parent_path().string(),
                ec.message()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        if (!copy_file_wrapper(source, target, process_status)) {
            return;
        }
    }
    if (ec) {
        std::string msg = fmt::format(
            "{}: Error traversing directory {}: {}",
            std::string(__FUNCTION__),
            source_vendor_dir.string(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }
}

void PresetUpdaterRepositorySync::stage_installed_vendor_from_resources(
    const boost::filesystem::path& source_dir,
    const AbstractPresetUpdaterRepository* repo,
    PresetUpdaterProcessStatus* process_status,
    const PresetUpdaterIndex& source_index
) const
{
    const fs::path installed_vendor_dir =
        local_vendor_path(repo->descriptor().id, source_index.vendor());
    const fs::path installed_vendor_yaml = installed_vendor_dir / "vendor.yaml";

    const fs::path update_sync_vendor_dir = fs::path(data_dir())
        / "update_sync"
        / repo->descriptor().id
        / source_index.vendor();
    const fs::path update_sync_index_path = fs::path(data_dir())
        / "update_sync"
        / repo->descriptor().id
        / (source_index.vendor() + ".idx"); // index is outside vendor folder.
    const fs::path update_sync_vendor_yaml = update_sync_vendor_dir / "vendor.yaml";

    const fs::path source_vendor_dir  = source_dir / source_index.vendor();
    const fs::path source_vendor_yaml = source_vendor_dir / "vendor.yaml";

    boost::system::error_code ec;
    Preset::IO::HwConfigLoader hw_config_loader;

    ASSERT(fs::exists(source_index.path()));
    ASSERT(fs::exists(source_vendor_dir) && fs::is_directory(source_vendor_dir));
    ASSERT(fs::exists(source_vendor_yaml) && fs::is_regular_file(source_vendor_yaml));

    // Recommended version of vendor
    const PresetUpdaterIndex::const_iterator recommended = source_index.recommended();
    PresetUpdaterIndex installed_index; // recommneded is an iterator - once its index object stops existing it ivalidates.
    PresetUpdaterIndex update_sync_index;
    if (recommended == source_index.end()) {
        process_status->set_error(
            fmt::format(
                "No recommended version for vendor: {}, Index file might be corrupted.",
                source_index.vendor()
            )
        );
        return;
    }

    // Check if installed version is recommended
    ASSERT (fs::exists(installed_vendor_yaml));
    try {
        Domain::Preset::VendorData installed_vendor_data;
        installed_vendor_data = hw_config_loader.load(installed_vendor_yaml.string());
        Semver installed_version = Semver(installed_vendor_data.info.version);
        if (installed_version == recommended->config_version) {
            // installed version is recommended - nothing to do here
            // Check version staged in update_sync - if its not higher than index recommended - delete it.
            if (fs::exists(update_sync_vendor_yaml, ec) && !ec) {
                Domain::Preset::VendorData update_sync_vendor_data;
                try {
                    update_sync_vendor_data = hw_config_loader.load(update_sync_vendor_yaml.string());
                    Semver update_sync_version = Semver(update_sync_vendor_data.info.version);
                    if (update_sync_version > recommended->config_version) {
                        // staged version is recommended - nothing to do here
                        return;
                    }
                } catch (const std::exception& e) {
                    SPDLOG_ERROR(
                        "Failed to load vendor file {}: {}",
                        update_sync_vendor_yaml.string(),
                        e.what()
                    );
                }
                if (!fs::remove(update_sync_index_path, ec) && ec) {
                process_status->set_warning(
                    fmt::format(
                        "Failed to remove file {}: {}",
                        update_sync_index_path.string(),
                        ec.message()
                    )
                );
                }
                if (!fs::remove_all(update_sync_vendor_dir, ec) && ec) {
                    process_status->set_warning(
                        fmt::format(
                            "Failed to remove file {}: {}",
                            update_sync_vendor_dir.string(),
                            ec.message()
                        )
                    );
                }
            }

            return;
        }
    } catch (const std::exception& e) {
        std::string msg = fmt::format(
            "Failed to load vendor file {}: {}",
            installed_vendor_yaml.string(),
            e.what()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }


    // Check version staged in update_sync
    if (fs::exists(update_sync_index_path, ec) && !ec && fs::exists(update_sync_vendor_yaml, ec) && !ec) {
        bool loaded_update_sync_index = false;
        try {
            update_sync_index.load(update_sync_index_path);
            loaded_update_sync_index = true;
        } catch (const std::runtime_error&) {
            process_status->set_warning(
                "Failed to load index " + update_sync_index_path.string()
            );
        }
        if (loaded_update_sync_index) {
            const PresetUpdaterIndex::const_iterator update_sync_recommended = update_sync_index.recommended();
            if (update_sync_recommended->config_version > recommended->config_version) {
                // Update sync has some more recent data - nothing to do here
                return;

            }
            // read version of staged
            Domain::Preset::VendorData update_sync_vendor_data;
            try {
                update_sync_vendor_data = hw_config_loader.load(update_sync_vendor_yaml.string());
                Semver update_sync_version = Semver(update_sync_vendor_data.info.version);
                if (update_sync_version == recommended->config_version) {
                    // staged version is recommended - nothing to do here
                    return;
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR(
                    "Failed to load vendor file {}: {}",
                    update_sync_vendor_yaml.string(),
                    e.what()
                );
            }
        }
    }

    // Now we know that data in update_sync are not the recommended version
    // We also know that data in source (resources) are usable data - they came with the installation of this binary.
    // We simply delete data in update_sync and replace them with source. 
    if (!fs::remove(update_sync_index_path, ec) && ec) {
        process_status->set_warning(
            fmt::format(
                "Failed to remove file {}: {}",
                update_sync_index_path.string(),
                ec.message()
            )
        );
    }
    if (!fs::remove_all(update_sync_vendor_dir, ec) && ec) {
        process_status->set_warning(
            fmt::format(
                "Failed to remove file {}: {}",
                update_sync_vendor_dir.string(),
                ec.message()
            )
        );
    }
    // Continue to copy data from source.

    // delete previous content of update_sync_vendor_dir
    fs::remove_all(update_sync_vendor_dir, ec);

    // Copy source to update_sync

    if (!fs::create_directories(update_sync_vendor_dir, ec) && ec) {
        process_status->set_error(
            "Failed to create vendor dir " + update_sync_vendor_dir.string() + ". " + ec.message()
        );
        return;
    }

    if (!copy_file_wrapper(source_index.path(), update_sync_index_path, process_status)) {
        return;
    }
    for (const auto& entry : fs::recursive_directory_iterator(source_vendor_dir, ec)) {
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }
        const fs::path source  = entry.path();
        fs::path relative_path = fs::relative(source, source_vendor_dir);
        const fs::path target  = update_sync_vendor_dir / relative_path;
        if (!fs::create_directories(target.parent_path(), ec) && ec) {
            std::string msg = fmt::format(
                "Failed to create target directory {}: {}. Staging update has failed.",
                target.parent_path().string(),
                ec.message()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        if (!copy_file_wrapper(source, target, process_status)) {
            return;
        }
    }
    if (ec) {
        std::string msg = fmt::format(
            "{}: Error traversing directory {}: {}",
            std::string(__FUNCTION__),
            source_vendor_dir.string(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }
}

void PresetUpdaterRepositorySync::sync_repository(
    const boost::filesystem::path& temp_dir,
    const AbstractPresetUpdaterRepository* repo,
    PresetUpdaterProcessStatus* process_status
) const
{
    // SPDLOG_INFO(__FUNCTION__);
    process_status->set_target(repo->descriptor().id + " repository");

    boost::system::error_code ec;

    // Each repo has its own subdir.
    if (!fs::create_directory(temp_dir, ec) && ec) {
        std::string msg = fmt::format(
            "Failed to check updates for source {}. Failed to create temp directory {}: {}.",
            repo->descriptor().id,
            temp_dir.string(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        DEBUG_ASSERT(false);
        return;
    }

    fs::path archive_path(temp_dir / "vendor_indices.zip");
    // Download profiles repo zip
    if (!repo->get_archive(archive_path, process_status)) {
        std::string msg = fmt::format(
            "Failed to check updates for source {}. Failed to download vendor profiles archive zip.",
            repo->descriptor().id
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return;
    }
    if (process_status->get_canceled()) {
        return;
    }
    /*
    enum class VendorStatus
    {
        Unknown, // default state after unzipping index
        InTemp, // Vendor not installed and its recommended version is to be downloaded.
        //IN_RESOURCES, // Vendor not installed and its recommended version is in resources.
        Installed, // Vendor is installed and it is its recommended version.
        NewVersion, // Vendor is installed, but recommended version is different, it is to be downloaded.
    };
    */
    std::vector<std::string> vendors_list;

    // desired temp_dir appearance
    // temp_dir
    // |- vendor1 # directory of vendor1 if needed
    // |- data # all data needed
    // |- vendor3 # directory of vendor3 if needed (vendor2 apparently needs nothing to download)
    // |-vendor_indices.zip # zip file that changes for each repository
    // |-vendor1.idx
    // |-vendor2.idx
    // |-vendor3.idx
    //

    // Unzip archive to temp_dir
    mz_zip_archive archive;
    mz_zip_zero_struct(&archive);
    if (!Slic3r::open_zip_reader(&archive, archive_path.string())) {
        std::string msg = fmt::format(
            "Failed to check updates for source {}. Couldn't open zipped bundle.",
            repo->descriptor().id
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        DEBUG_ASSERT(false);
        return;
    } else {
        mz_uint num_entries = mz_zip_reader_get_num_files(&archive);
        // loop the entries
        mz_zip_archive_file_stat stat;
        for (mz_uint i = 0; i < num_entries; ++i) {
            if (mz_zip_reader_file_stat(&archive, i, &stat)) {
                std::string name(stat.m_filename);

                if (mz_zip_reader_is_file_a_directory(&archive, i)) {
                    std::string msg = fmt::format("Skipping directory entry: {}", name);
                    SPDLOG_INFO(msg);
                    continue;
                }

                if (stat.m_uncomp_size > 0) {
                    std::string buffer((size_t) stat.m_uncomp_size, 0);
                    mz_bool res = mz_zip_reader_extract_to_mem(
                        &archive,
                        stat.m_file_index,
                        (void*) buffer.data(),
                        (size_t) stat.m_uncomp_size,
                        0
                    );
                    if (res == 0) {
                        std::string msg = fmt::format("Failed to unzip {}", stat.m_filename);
                        SPDLOG_ERROR(msg);
                        process_status->set_warning(msg);
                        continue;
                    }
                    // create file from buffer
                    fs::path tmp_path(temp_dir / (name + ".tmp"));
                    if (!fs::exists(tmp_path.parent_path(), ec) || ec) {
                        std::string msg = fmt::format(
                            "Failed to unzip file {}. Directories are not supported. Skipping file.",
                            name
                        );
                        SPDLOG_ERROR(msg);
                        process_status->set_warning(msg);
                        continue;
                    }
                    fs::path target_path(temp_dir / name);
                    boost::nowide::fstream file(
                        tmp_path,
                        std::ios::out | std::ios::binary | std::ios::trunc
                    );
                    file.write(buffer.c_str(), buffer.size());
                    file.close();
                    boost::system::error_code ec;
                    bool exists = fs::exists(tmp_path, ec);
                    if (!exists || ec) {
                        std::string msg = fmt::format(
                            "Failed to find unzipped file at {}. Terminating Preset updater synchronization.",
                            tmp_path.string()
                        );
                        SPDLOG_ERROR(msg);
                        process_status->set_warning(msg);
                        Slic3r::close_zip_reader(&archive);
                        return;
                    }
                    auto result = safe_move(tmp_path, target_path);
                    if (ec || !result.has_value()) {
                        std::string msg = fmt::format(
                            "Failed to rename unzipped file at {}. Terminating Preset updater synchorinzation. Error message: {}",
                            tmp_path.string(),
                            ec ? ec.message() : result.error()
                        );
                        SPDLOG_ERROR(msg);
                        process_status->set_warning(msg);
                        Slic3r::close_zip_reader(&archive);
                        return;
                    }

                    if (name.substr(name.size() - 3) == "idx") {
                        vendors_list.emplace_back(name);
                    }
                }
            }
        }
        Slic3r::close_zip_reader(&archive);
    }

    // Now we have vendors_list, but we need index_db.
    std::vector<PresetUpdaterIndex> index_db;
    try {
        index_db = load_vendors_db_filtered(temp_dir, vendors_list);
    } catch (const std::exception& e) {
        std::string msg = fmt::format(
            "Loading index db of {} has failed {}",
            repo->descriptor().id,
            e.what()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return;
    }

    for (const auto& index : index_db) {
        if (process_status->get_canceled()) {
            return;
        }
        process_status->set_warning_target(repo->descriptor().id, index.vendor());
        if (!is_vendor_installed(index.vendor(), repo->descriptor().id)) {
            // For not installed vendor we need to download its recommended version.
            sync_not_installed_vendor(temp_dir, repo, process_status, index);
            continue;
        }
        // if installed, it can still mean there is different recommended version.
        sync_installed_vendor(temp_dir, repo, process_status, index);
    }
    process_status->clear_warning_target();
}

void PresetUpdaterRepositorySync::sync_not_installed_vendor(
    const boost::filesystem::path& temp_path,
    const AbstractPresetUpdaterRepository* repo,
    PresetUpdaterProcessStatus* process_status,
    const PresetUpdaterIndex& index
) const
{
    const fs::path temp_vendor_dir_path        = temp_path / index.vendor();
    const fs::path update_sync_vendor_dir_path = fs::path(data_dir())
        / "update_sync"
        / repo->descriptor().id
        / index.vendor();
    const fs::path temp_manifest_path = temp_vendor_dir_path / (index.vendor() + ".manifest");
    const fs::path update_sync_manifest_path = update_sync_vendor_dir_path
        / (index.vendor() + ".manifest");
    const fs::path index_update_sync_path = update_sync_vendor_dir_path.parent_path()
        / index.path().filename();
    boost::system::error_code ec;

    ASSERT(fs::exists(index.path()));

    // Recommended version of vendor
    const PresetUpdaterIndex::const_iterator recommended = index.recommended();
    if (recommended == index.end()) {
        std::string msg = fmt::format(
            "No recommended version for vendor: {}, Index file might be corrupted.",
            index.vendor()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        // DEBUG_ASSERT(false);
        return;
    }

    // Create directory in temp. Delete all previous content if already extists.
    if (fs::create_directories(temp_vendor_dir_path, ec) || !ec) {
        for (fs::directory_iterator it(temp_vendor_dir_path); it != fs::directory_iterator(); ++it) {
            fs::remove_all(it->path());
        }
    } else {
        std::string msg = fmt::format(
            "Failed to create target directory {} for vendor {}: {}. Staging update has failed.",
            temp_vendor_dir_path.string(),
            index.vendor(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return;
    }

    // Get version manifest
    const std::string source_subpath = fmt::format(
        "{}/{}/manifest.json",
        index.vendor(),
        recommended->config_version.to_string()
    );
    if (!repo->get_version_manifest(source_subpath, temp_manifest_path, process_status)) {
        std::string msg = fmt::format(
            "{}: Failed to get file {}. Staging update has failed.",
            std::string(__FUNCTION__),
            source_subpath
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return;
    }

    // read the version manifest
    std::map<std::string, std::string> files_in_version_manifest = read_version_manifest(
        temp_manifest_path,
        process_status
    );
    SPDLOG_INFO(
        "{}/{}: {} files",
        repo->descriptor().id,
        index.vendor(),
        std::to_string(files_in_version_manifest.size())
    );

    // Create vendor dir in update_sync (leave files in it if already exists)
    if (!fs::create_directories(update_sync_vendor_dir_path, ec) && ec) {
        std::string msg = fmt::format(
            "Failed to create target directory {} for vendor {}: {}. Staging update has failed.",
            update_sync_vendor_dir_path.string(),
            index.vendor(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return;
    }

    // Create list of files in update_sync
    std::map<std::string, PresetUpdaterFileHash> files_in_update_sync;
    for (const auto& entry : fs::recursive_directory_iterator(update_sync_vendor_dir_path, ec)) {
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }
        const fs::path& path   = entry.path();
        fs::path relative_path = fs::relative(path, update_sync_vendor_dir_path);
        files_in_update_sync.emplace(relative_path.string(), file_hash(path, process_status));
    }
    if (ec) {
        std::string msg = fmt::format(
            "{}: Error traversing directory {}: {}",
            std::string(__FUNCTION__),
            update_sync_vendor_dir_path.string(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // Compare files_in_version_manifest and files_in_update_sync
    std::vector<std::string> files_to_delete;
    std::map<std::string, std::string> files_to_download;
    for (const auto& [name, hash] : files_in_version_manifest) {
        auto it = files_in_update_sync.find(name);
        if (it == files_in_update_sync.end()) {
            // Only in files_in_version_manifest -> download
            files_to_download.emplace(name, hash);
        } else if (it->second != hash) {
            // In both files_in_version_manifest and files_in_update_sync and different hash -> delete and download
            files_to_delete.push_back(name);
            files_to_download.emplace(name, hash);
        }
        // In both files_in_version_manifest and files_in_update_sync and same hash = do nothing
    }
    for (const auto& [name, hash] : files_in_update_sync) {
        if (!files_in_version_manifest.contains(name)) {
            // Only in files_in_update_sync -> delete
            files_to_delete.push_back(name);
        }
    }

    // Delete selected files in update_sync
    for (const std::string& filename : files_to_delete) {
        fs::path path(update_sync_vendor_dir_path / filename);
        ASSERT(fs::exists(path) && fs::is_regular_file(path));
        if (!fs::remove(path, ec) || ec) {
            std::string msg = fmt::format(
                "{}: Failed to remove file {}",
                std::string(__FUNCTION__),
                path.string()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
        }
    }

    // Download selected files to temp and move to update_sync
    for (const auto& [name, hash] : files_to_download) {
        const fs::path target_path(temp_vendor_dir_path / name);
        if (!fs::create_directories(target_path.parent_path(), ec) && ec) {
            std::string msg = fmt::format(
                "Failed to create target directory {} for vendor {}: {}. Staging update has failed.",
                target_path.parent_path().string(),
                index.vendor(),
                ec.message()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        const std::string source_subpath = fmt::format(
            "{}/{}/{}",
            index.vendor(),
            recommended->config_version.to_string(),
            name
        );
        if (!repo->get_file(source_subpath, target_path, hash, process_status)) {
            std::string msg = fmt::format(
                "{}: Failed to get file {}. Staging update has failed.",
                std::string(__FUNCTION__),
                source_subpath
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        // move to update_sync
        const fs::path dest_path(update_sync_vendor_dir_path / name);
        if (!fs::create_directories(dest_path.parent_path(), ec) && ec) {
            std::string msg = fmt::format(
                "Failed to create target directory {} for vendor {}: {}. Staging update has failed.",
                dest_path.parent_path().string(),
                index.vendor(),
                ec.message()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        auto result = safe_move(target_path, dest_path);
        if (!result) {
            std::string msg = fmt::format(
                "{}: Failed to move file {} to {}: {}",
                std::string(__FUNCTION__),
                target_path.string(),
                dest_path.string(),
                ec ? ec.message() : result.error()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
        }
    }

    // Move index to stage_sync
    auto result = safe_move(index.path(), index_update_sync_path);
    if (!result) {
        std::string msg = fmt::format(
            "{}: Failed to move file {}: {}",
            std::string(__FUNCTION__),
            index.path().string(),
            result.error()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // Move manifest file to update_sync
    result = safe_move(temp_manifest_path, update_sync_manifest_path);
    if (!result) {
        std::string msg = fmt::format(
            "{}: Failed to move file {}: {}",
            std::string(__FUNCTION__),
            index.path().string(),
            result.error()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // Cleanup temp
    if (!fs::remove_all(temp_vendor_dir_path, ec) || ec) {
        std::string msg = fmt::format(
            "{}: Failed to delete directory {}.",
            std::string(__FUNCTION__),
            temp_vendor_dir_path.string()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }
}

void PresetUpdaterRepositorySync::sync_installed_vendor(
    const boost::filesystem::path& temp_path,
    const AbstractPresetUpdaterRepository* repo,
    PresetUpdaterProcessStatus* process_status,
    const PresetUpdaterIndex& index
) const
{
    const fs::path installed_vendor_dir_path =
        local_vendor_path(repo->descriptor().id, index.vendor());
    const fs::path update_sync_vendor_dir_path = fs::path(data_dir())
        / "update_sync"
        / repo->descriptor().id
        / index.vendor();
    const fs::path temp_vendor_dir_path = temp_path / index.vendor();
    const fs::path temp_manifest_path   = temp_vendor_dir_path / (index.vendor() + ".manifest");
    const fs::path update_sync_manifest_path = update_sync_vendor_dir_path
        / (index.vendor() + ".manifest");
    const fs::path update_sync_vendor_yaml_path = update_sync_vendor_dir_path / "vendor.yaml";
    const fs::path index_update_sync_path = update_sync_vendor_dir_path.parent_path()
        / index.path().filename();
    const fs::path installed_vendor_yaml_path = installed_vendor_dir_path / "vendor.yaml";
    boost::system::error_code ec;

    ASSERT(fs::exists(index.path()));
    ASSERT(fs::exists(installed_vendor_dir_path) && fs::is_directory(installed_vendor_dir_path));
    ASSERT(fs::exists(installed_vendor_yaml_path));

    // Current installed version
    Preset::IO::HwConfigLoader hw_config_loader;
    Domain::Preset::VendorData vendor_data;
    try {
        vendor_data = hw_config_loader.load(installed_vendor_yaml_path.string());
    } catch (const std::exception& e) {
        process_status->set_error(
            fmt::format("Failed to read vendor file {}: {}", installed_vendor_yaml_path.string(), e.what())
        );
        return;
    }
    Semver installed_version = Semver(vendor_data.info.version);

    // Recommended version of vendor
    const PresetUpdaterIndex::const_iterator recommended = index.recommended();
    if (recommended == index.end()) {
        process_status->set_warning(
            fmt::format(
                "No recommended version for vendor: {}, Index file might be corrupted.",
                index.vendor()
            )
        );
        DEBUG_ASSERT(false);
        return;
    }
    const PresetUpdaterIndex::const_iterator vendor_current_version_it = index.find(installed_version);
    if (recommended == vendor_current_version_it) {
        SPDLOG_INFO(
            "Vendor {}/{} has installed recommended version.",
            repo->descriptor().id,
            index.vendor()
        );

        // Delete staged in update_sync. It is not needed.
        if (!fs::remove(index_update_sync_path, ec) && ec) {
            process_status->set_warning(
                fmt::format(
                    "Failed to remove file {}: {}",
                    index_update_sync_path.string(),
                    ec.message()
                )
            );
        }
        if (!fs::remove_all(update_sync_vendor_dir_path, ec) && ec) {
            process_status->set_warning(
                fmt::format(
                    "Failed to remove file {}: {}",
                    installed_vendor_dir_path.string(),
                    ec.message()
                )
            );
        }   
        return;
    }

    // Create directory in temp. Delete all previous content if already extists.
    if (fs::create_directories(temp_vendor_dir_path, ec) || !ec) {
        for (fs::directory_iterator it(temp_vendor_dir_path); it != fs::directory_iterator(); ++it) {
            fs::remove_all(it->path());
        }
    } else {
        std::string msg = fmt::format(
            "Failed to create target directory {} for vendor {}: {}. Staging update has failed.",
            temp_vendor_dir_path.string(),
            index.vendor(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return;
    }

    // Get version manifest
    const std::string source_subpath = fmt::format(
        "{}/{}/manifest.json",
        index.vendor(),
        recommended->config_version.to_string()
    );
    if (!repo->get_version_manifest(source_subpath, temp_manifest_path, process_status)) {
        std::string msg = fmt::format(
            "{}: Failed to get file {}. Staging update has failed.",
            std::string(__FUNCTION__),
            source_subpath
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return;
    }

    // read the version manifest
    std::map<std::string, std::string> files_in_version_manifest = read_version_manifest(
        temp_manifest_path,
        process_status
    );
    SPDLOG_INFO(
        "{}/{}: {} files",
        repo->descriptor().id,
        index.vendor(),
        std::to_string(files_in_version_manifest.size())
    );

    // Create list of files in installed_vendor_dir_path and subdirectories
    std::map<std::string, PresetUpdaterFileHash> files_in_installed;
    for (const auto& entry : fs::recursive_directory_iterator(installed_vendor_dir_path, ec)) {
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }
        const fs::path& path   = entry.path();
        fs::path relative_path = fs::relative(path, installed_vendor_dir_path);
        files_in_installed.emplace(relative_path.string(), file_hash(path, process_status));
    }
    if (ec) {
        std::string msg = fmt::format(
            "{}: Error traversing directory {}: {}",
            std::string(__FUNCTION__),
            installed_vendor_dir_path.string(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // Compare files_in_version_manifest and files_in_update_sync
    std::map<std::string, std::string> files_to_download_against_installed;
    for (const auto& [name, hash] : files_in_version_manifest) {
        auto it = files_in_installed.find(name);
        if (it == files_in_installed.end() || it->second != hash) {
            // Not in installed or not same hash -> download
            files_to_download_against_installed[name] = hash;
        }
        // In both files_in_version_manifest and files_in_update_sync and same hash = do nothing
    }

    // Create vendor dir in update_sync (leave files in it if already exists)
    if (!fs::create_directories(update_sync_vendor_dir_path, ec) && ec) {
        std::string msg = fmt::format(
            "Failed to create target directory {} for vendor {}: {}. Staging update has failed.",
            update_sync_vendor_dir_path.string(),
            index.vendor(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
        return;
    }

    // Create list of files in update_sync
    std::map<std::string, PresetUpdaterFileHash> files_in_update_sync;
    for (const auto& entry : fs::recursive_directory_iterator(update_sync_vendor_dir_path, ec)) {
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }
        const fs::path& path   = entry.path();
        fs::path relative_path = fs::relative(path, update_sync_vendor_dir_path);
        files_in_update_sync.emplace(relative_path.string(), file_hash(path, process_status));
    }
    if (ec) {
        std::string msg = fmt::format(
            "{}: Error traversing directory {}: {}",
            std::string(__FUNCTION__),
            installed_vendor_dir_path.string(),
            ec.message()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // Compare files_to_download_against_installed and files_in_update_sync
    std::vector<std::string> files_to_delete;
    std::map<std::string, std::string> files_to_download;
    for (const auto& [name, hash] : files_to_download_against_installed) {
        auto it = files_in_update_sync.find(name);
        if (it == files_in_update_sync.end()) {
            // Only in files_to_download_against_installed -> download
            files_to_download.emplace(name, hash);
        } else if (it->second != hash) {
            // In both files_to_download_against_installed and files_in_update_sync and different hash -> delete and download
            files_to_delete.push_back(name);
            files_to_download.emplace(name, hash);
        }
        // In both files_to_download_against_installed and files_in_update_sync and same hash = do nothing
    }
    for (const auto& [name, hash] : files_in_update_sync) {
        if (!files_to_download_against_installed.contains(name)) {
            // Only in files_in_update_sync -> delete
            files_to_delete.push_back(name);
        }
    }

    // Delete selected files in update_sync
    for (const std::string& filename : files_to_delete) {
        fs::path path(update_sync_vendor_dir_path / filename);
        ASSERT(fs::exists(path) && fs::is_regular_file(path));
        if (!fs::remove(path, ec) || ec) {
            std::string msg = fmt::format(
                "{}: Failed to remove file {}",
                std::string(__FUNCTION__),
                path.string()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
        }
    }

    // Download selected files to temp and move to update_sync
    for (const auto& [name, hash] : files_to_download) {
        const fs::path target_path(temp_vendor_dir_path / name);
        if (!fs::create_directories(target_path.parent_path(), ec) && ec) {
            std::string msg = fmt::format(
                "Failed to create target directory {} for vendor {}: {}. Staging update has failed.",
                target_path.parent_path().string(),
                index.vendor(),
                ec.message()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        const std::string source_subpath = fmt::format(
            "{}/{}/{}",
            index.vendor(),
            recommended->config_version.to_string(),
            name
        );
        if (!repo->get_file(source_subpath, target_path, hash, process_status)) {
            std::string msg = fmt::format(
                "{}: Failed to get file {}. Staging update has failed.",
                std::string(__FUNCTION__),
                source_subpath
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        // move to update_sync
        const fs::path dest_path(update_sync_vendor_dir_path / name);
        if (!fs::create_directories(dest_path.parent_path(), ec) && ec) {
            std::string msg = fmt::format(
                "Failed to create target directory {} for vendor {}: {}. Staging update has failed.",
                dest_path.parent_path().string(),
                index.vendor(),
                ec.message()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
            return;
        }
        auto result = safe_move(target_path, dest_path);
        if (!result) {
            std::string msg = fmt::format(
                "{}: Failed to move file {} to {}: {}",
                std::string(__FUNCTION__),
                target_path.string(),
                dest_path.string(),
                result.error()
            );
            SPDLOG_ERROR(msg);
            process_status->set_warning(msg);
        }
    }

    // Move index to update_sync
    auto result = safe_move(index.path(), index_update_sync_path);
    if (!result) {
        std::string msg = fmt::format(
            "{}: Failed to move file {}: {}",
            std::string(__FUNCTION__),
            index.path().string(),
            result.error()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // Move manifest file to update_sync
    result = safe_move(temp_manifest_path, update_sync_manifest_path);
    if (!result) {
        std::string msg = fmt::format(
            "{}: Failed to move file {}: {}",
            std::string(__FUNCTION__),
            index.path().string(),
            result.error()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }

    // Cleanup temp
    if (!fs::remove_all(temp_vendor_dir_path, ec) || ec) {
        std::string msg = fmt::format(
            "{}: Failed to delete directory {}.",
            std::string(__FUNCTION__),
            temp_vendor_dir_path.string()
        );
        SPDLOG_ERROR(msg);
        process_status->set_warning(msg);
    }
}

} // namespace Slic3r::Biz::PresetUpdater
