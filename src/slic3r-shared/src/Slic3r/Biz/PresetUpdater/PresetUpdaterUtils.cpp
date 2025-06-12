#include "PresetUpdaterUtils.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterIndex.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterProcessStatus.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterReconfigurationList.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterVendorProfile.hpp"
#include "Slic3r/Biz/CopyFile.hpp"
#include "Slic3r/Biz/Directories.hpp"

#include "Slic3r/Exception.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/system/error_code.hpp>
#include "fmt/format.h"

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PresetUpdater {

void copy_file_fix(const fs::path& source, const fs::path& target)
{
	SPDLOG_INFO("PresetUpdater: Copying {} -> {}", source.string(), target.string());
	std::string error_message;
	Utils::CopyFileResult cfr = Utils::copy_file(source.string(), target.string(), error_message, false);
	if (cfr != Utils::CopyFileResult::Success) {
		SPDLOG_ERROR("Copying failed: {}", error_message);
		throw Slic3r::CriticalException(fmt::format(
				"Copying of file {} to {} failed: {}",
				source.string(), target.string(), error_message));
	}
	// Permissions should be copied from the source file by copy_file(). We are not sure about the source
	// permissions, let's rewrite them with 644.
	static constexpr const auto perms = fs::owner_read | fs::owner_write | fs::group_read | fs::others_read;
	fs::permissions(target, perms);
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
    for (auto &dir_entry : fs::directory_iterator(archive_path, ec)) {
        if (is_idx_file(dir_entry)) {
    	    PresetUpdaterIndex idx;
            try {
        	    idx.load(dir_entry.path());
            } catch (const std::runtime_error &err) {
                errors_cumulative += err.what();
                errors_cumulative += "\n";
                continue;
    	    }
            if (std::find_if(index_db.begin(), index_db.end(), [idx](const PresetUpdaterIndex& index) { return idx.vendor() == index.vendor();}) == index_db.end())
                index_db.emplace_back(std::move(idx));
        }
    }
    if (! errors_cumulative.empty())
        throw Slic3r::RuntimeError(errors_cumulative);
    return index_db;
}

std::vector<PresetUpdaterIndex> load_vendors_db_filtered(const boost::filesystem::path& from_path, const std::vector<std::string>& filter)
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
    for (auto &dir_entry : source_directory_iterator) {
        if (is_idx_file(dir_entry)) {
            if (std::find(filter.begin(), filter.end(), dir_entry.path().filename().string()) == filter.end()) {
                // not an index from this archive
                continue;
            }
            PresetUpdaterIndex idx;
            try {
        	    idx.load(dir_entry.path());
            } catch (const std::runtime_error &err) {
                errors_cumulative += err.what();
                errors_cumulative += "\n";
                continue;
            }
            if (std::find_if(index_db.begin(), index_db.end(), [idx](const PresetUpdaterIndex& index) { return idx.vendor() == index.vendor();}) == index_db.end()) {
                index_db.emplace_back(std::move(idx));
            } else {
                assert(false);
            }
        }
    }
    //assert (index_db.size() == filter.size());
    if (! errors_cumulative.empty())
        throw Slic3r::RuntimeError(errors_cumulative);
    return index_db;
}



namespace {

void perform_downgrades(const std::vector<VendorReconfiguration>& downgrades, PresetUpdaterProcessStatus* process_status)
{
    for (const VendorReconfiguration& downgrade : downgrades) {
        SPDLOG_INFO("Deleting incompatible bundle {}", downgrade.vendor_id); 
        fs::path dir_path = fs::path(Utils::data_dir()) / "profiles" / "local" / "vendor" / downgrade.vendor_archive_id / downgrade.vendor_id;
        fs::path idx_path = fs::path(Utils::data_dir()) / "profiles" / "local" / "vendor" / downgrade.vendor_archive_id / (downgrade.vendor_id + ".idx");
        boost::system::error_code ec;
        if (!fs::remove_all(dir_path, ec) || ec) {
            process_status->set_error(fmt::format("Failed to remove directory {} while performing downgrade. {}",dir_path.string(), ec.message()));
            return;
        }
        if (!fs::remove(idx_path, ec) || ec) {
            process_status->set_error(fmt::format("Failed to remove index {} while performing downgrade. {}",idx_path.string(), ec.message()));
            return;
        }
    }
}

void deep_copy(const fs::path& source, const fs::path& destination) 
{
    boost::system::error_code ec;
    if (!fs::exists(source, ec) || ec || !fs::is_directory(source, ec) || ec) {
        throw Slic3r::CriticalException("Copying files has failed. " + source.string() + ". " + ec.message());
        return;
    }

    if (!fs::exists(destination, ec)) {
        ec.clear();
        if (!fs::create_directories(destination, ec)) {
            throw Slic3r::CriticalException("Failed to create directory. " + destination.string() + ". " + ec.message());
            return;
        }
    }
    
    ec.clear();
    for (const auto& it : fs::directory_iterator(source, ec)) { 
        fs::path new_path = destination / it.path().filename();
        ec.clear();
        if (fs::is_directory(it.path(), ec)) {
            deep_copy(it.path(), new_path);
        } else {
            copy_file_fix(it.path(), new_path);
        }
    }
}

void perform_updates(const std::vector<VendorReconfiguration>& updates, PresetUpdaterProcessStatus* process_status)
{
    const fs::path profile_local_path = fs::path(Utils::data_dir()) / "profiles" / "local" / "vendor";
    const fs::path update_sync_path = fs::path(Utils::data_dir()) / "update_sync";
    boost::system::error_code ec;
    for (const VendorReconfiguration& update : updates) {
        SPDLOG_INFO("Updating bundle {}", update.vendor_id);

        const fs::path vendor_dest_dir_path = profile_local_path / update.vendor_archive_id / update.vendor_id;
        const fs::path vendor_dest_idx_path = profile_local_path / update.vendor_archive_id / (update.vendor_id + ".idx");
        const fs::path vendor_source_dir_path = update_sync_path / update.vendor_archive_id / update.vendor_id;
        const fs::path vendor_source_idx_path = update_sync_path / update.vendor_archive_id / (update.vendor_id + ".idx");

        ec.clear();
        if (!fs::remove_all(vendor_dest_dir_path, ec) || ec) {
            process_status->set_error(fmt::format("Failed to remove directory {} while performing update. {}",vendor_dest_dir_path.string(), ec.message()));
            return;
        }
        if (!fs::remove(vendor_dest_idx_path, ec) || ec) {
            process_status->set_error(format("Failed to remove index {} while performing update. {}",vendor_dest_dir_path.string(), ec.message()));
            return;
        }

        try
        {
            deep_copy(vendor_source_dir_path, vendor_dest_dir_path);
            copy_file_fix(vendor_source_idx_path, vendor_dest_idx_path);
        }
        catch (const Slic3r::CriticalException& e)
        {
            process_status->set_error(e.what());
            return;
        }


    }
}
} // namespace

void check_forced_reconfigurations(PresetUpdaterReconfigurationList& results, PresetUpdaterProcessStatus* process_status)
{
    const fs::path profile_local_path = fs::path(Utils::data_dir()) / "profiles" / "local" / "vendor";
    boost::system::error_code ec;

    if (!fs::exists(profile_local_path, ec) || ec) {
        process_status->set_error(profile_local_path.string() + " does not exists. " + ec.message());
        return;
    }
    if (!fs::is_directory(profile_local_path, ec) || ec) {
        process_status->set_error(profile_local_path.string() + " is not directory. " + ec.message());
        return;
    }

    // vendors are inside archive_id dir
    ec.clear();
    for (auto &archive_dir : fs::directory_iterator(profile_local_path, ec)) {
        if (process_status->get_canceled()) {
            return;
        }
        ec.clear();
        if (!fs::is_directory(archive_dir.path(), ec) || ec) {
            continue;
        }
        const std::string archive_id = archive_dir.path().filename().string();

        std::vector<PresetUpdaterIndex> index_db = load_vendors_db(archive_dir.path());
        for (const auto &index : index_db) {
            const fs::path bundle_path = archive_dir.path() / index.vendor() / (index.vendor() + ".ini");
            ec.clear();

            if (!fs::exists(bundle_path, ec) || ec) {
                process_status->set_warning(fmt::format("File does not exists {}. {}", bundle_path.string(), ec.message()));
                continue;
            }

            // Perform a basic load and check the version of the installed preset bundle.
		    PresetUpdaterVendorProfile vp;
		    try {
			    vp = PresetUpdaterVendorProfile::from_ini(bundle_path, false);
		    }
		    catch (const std::exception& e) {
			    SPDLOG_ERROR("Corrupted profile file for vendor {} at {}, message: {}", index.vendor(), bundle_path.string(), e.what());
			    continue;
		    }
            // Recommended version of vendor
		    const PresetUpdaterIndex::const_iterator recommended = index.recommended();
		    if (recommended == index.end()) {
			    SPDLOG_ERROR("No recommended version for vendor: {}, Index file might be corrupted.", index.vendor());
                assert(false);
			    continue;
		    }
            const PresetUpdaterIndex::const_iterator vendor_current_version_it = index.find(vp.config_version);
		    const bool ver_current_found = vendor_current_version_it != index.end();

		    SPDLOG_INFO("Vendor: {}, version installed: {}{}, recommended version: {}",
			    vp.name,
			    vp.config_version.to_string(),
			    (ver_current_found ? "" : " (not found in index!)"),
			    recommended->config_version.to_string());

            if (! ver_current_found) {
			    // Any published config shall be always found in the latest config index.
			    SPDLOG_ERROR("Preset bundle `{}` version not found in index: {}", index.vendor(), vp.config_version.to_string());
                results.emplace_back(VendorReconfigurationState::NotInIndex, vp.id, archive_id, Slic3r::Semver(), recommended->config_version, recommended->comment, std::string(), std::string());
			    continue;
		    }

            if (vendor_current_version_it->is_current_slic3r_supported()){
                // This slicer needs no forced reconfigurations
                SPDLOG_ERROR("Preset bundle `{}` version {} needs no forced reconfiguration.",  index.vendor(), vp.config_version.to_string());
                continue;
            }

            if (vendor_current_version_it->is_current_slic3r_downgrade()) {
		        SPDLOG_WARN("Current Slic3r incompatible with installed bundle (forced downgrade): {}", bundle_path.string());
                results.emplace_back(VendorReconfigurationState::ForcedDowngrade, vp.id, archive_id, vendor_current_version_it->config_version, recommended->config_version, recommended->comment, std::string(), std::string());
			    continue;
            }
		    SPDLOG_WARN("Current Slic3r incompatible with installed bundle (forced update): {}", bundle_path.string());
            results.emplace_back(VendorReconfigurationState::ForcedUpdate, vp.id, archive_id, vendor_current_version_it->config_version, recommended->config_version, recommended->comment, std::string(), std::string());
        }
    }
}

void check_reconfigurations(PresetUpdaterReconfigurationList& results, PresetUpdaterProcessStatus* process_status)
{
    //SPDLOG_INFO(__FUNCTION__);

    const fs::path profile_local_path = fs::path(Utils::data_dir()) / "profiles" / "local" / "vendor";
    const fs::path update_sync_path = fs::path(Utils::data_dir()) / "update_sync";
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
    
    // vendors are inside archive_id dir
    for (auto &archive_dir : fs::directory_iterator(profile_local_path, ec)) {
        if (process_status->get_canceled()) {
            return;
        }
        ec.clear();
        if (!fs::is_directory(archive_dir.path(), ec) || ec) {
            continue;
        }
        const std::string archive_id = archive_dir.path().filename().string();

        std::vector<PresetUpdaterIndex> index_db = load_vendors_db(archive_dir.path());
        for (const auto &index : index_db) {
            // the installed bundles are in / "profiles" / "local"
            // the most recent index file should be in / "update_sync"
            // if index in / "profiles" / "local" is more recent use it instead
            // (should the index  in / "profiles" / "local" be rewritten if not most recent?)
            // then check installed bundles version and decide if and what type of reconfiguration
            
            // Compare update_sync index and index
            const fs::path update_sync_index_path = update_sync_path / archive_dir.path().filename() / index.path().filename();
            PresetUpdaterIndex most_recent_index;
            ec.clear();
            if (fs::exists(update_sync_index_path, ec) && !ec) {
                PresetUpdaterIndex update_sync_index;
                try {
        	        update_sync_index.load(update_sync_index_path);
                } catch (const std::runtime_error &err) {
                    process_status->set_warning("Failed to load index " + update_sync_index_path.string());
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

            const fs::path bundle_path = archive_dir.path() / index.vendor() / (index.vendor() + ".ini");
            // Perform a basic load and check the version of the installed preset bundle.
		    PresetUpdaterVendorProfile vp;
		    try {
			    vp = PresetUpdaterVendorProfile::from_ini(bundle_path, false);
		    }
		    catch (const std::exception& e) {
			    std::string message = fmt::format("Corrupted profile file for vendor {} at {}, message: {}", index.vendor(), bundle_path.string(), e.what());
                SPDLOG_ERROR(message);
                assert(false);
			    continue;
		    }
            // Recommended version of vendor
		    const PresetUpdaterIndex::const_iterator recommended = most_recent_index.recommended();
		    if (recommended == most_recent_index.end()) {
			    SPDLOG_ERROR("No recommended version for vendor: {}, Index file might be corrupted.", index.vendor());
                assert(false);
			    continue;
		    }
            const PresetUpdaterIndex::const_iterator vendor_current_version_it = most_recent_index.find(vp.config_version);
		    const bool current_found = vendor_current_version_it != most_recent_index.end();
            SPDLOG_INFO("Vendor: {}, version installed: {}{}, recommended version: {}",
			    vp.name,
			    vp.config_version.to_string(),
			    (current_found ? "" : " (not found in index!)"),
			    recommended->config_version.to_string());
            if (!current_found) {
                SPDLOG_ERROR("Current installed version for vendor: {} ({}) was not found in index %3%", vp.id, vp.config_version.to_string(), most_recent_index.path().string());
                // If current is not found in index, we need forced reconfiguration
                results.emplace_back(VendorReconfigurationState::NotInIndex,  vp.id, archive_id, Slic3r::Semver(), recommended->config_version, recommended->comment, std::string(), std::string());
                continue;
            }
            if (vendor_current_version_it->is_current_slic3r_supported()){
                // This slicer needs no forced reconfigurations
                assert (vp.config_version <= recommended->config_version);
                if (vp.config_version < recommended->config_version) {
                    // regular update
                    SPDLOG_DEBUG("Preset bundle `{}` version {} has update {}.",  index.vendor(), vp.config_version.to_string(), recommended->config_version.to_string());
                    results.emplace_back(VendorReconfigurationState::Update,  vp.id, archive_id, vendor_current_version_it->config_version, recommended->config_version, recommended->comment, std::string(), std::string());
                }
                continue;
            }

            if (vendor_current_version_it->is_current_slic3r_downgrade()) {
                // Forced downgrade
		        SPDLOG_WARN("Current Slic3r incompatible with installed bundle (forced downgrade): {}", bundle_path.string());
                results.emplace_back(VendorReconfigurationState::ForcedDowngrade,  vp.id, archive_id, vendor_current_version_it->config_version, recommended->config_version, recommended->comment, std::string(), std::string());
			    continue;
            }
            // Not supported and not downgrade = forced update
            SPDLOG_WARN("Current Slic3r incompatible with installed bundle (forced update): {}", bundle_path.string());
            results.emplace_back(VendorReconfigurationState::ForcedUpdate,  vp.id, archive_id, vendor_current_version_it->config_version.to_string(), recommended->config_version.to_string(), recommended->comment, std::string(), std::string());
        }
    }
}

void perform_reconfigurations(const PresetUpdaterReconfigurationList& reconfigurations, PresetUpdaterProcessStatus* process_status)
{
    if (!reconfigurations.forced_downgrades().empty()) {
        // In case of existing downgrade, we only perform downgrade. 
        // The rest will be done in following wizard.
        perform_downgrades(reconfigurations.forced_downgrades(), process_status);
        return;
    }
    // Forced and regular updates are now the same.
    // The difference was if this functions "must" or "should" be called.
    perform_updates(reconfigurations.forced_updates(), process_status);
    perform_updates(reconfigurations.regular_updates(), process_status);
}

} // namespace Slic3r::Biz::PresetUpdater