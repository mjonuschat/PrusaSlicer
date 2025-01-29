#include "PresetUpdater.hpp"

#include "PresetUpdaterUtils.hpp"
#include "PresetUpdaterProcessStatus.hpp"
#include "PresetUpdaterReconfigurations.hpp"
#include "VendorProfile.hpp"

#include "../../Utils/Utils.hpp"
#include "Version.hpp"
#include  "../../Utils/Format.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/trivial.hpp>

namespace fs = boost::filesystem;

namespace PresetManagement {

namespace {
}
void PresetUpdater::check_forced_reconfigurations(ReconfigurationsList& results, PresetUpdaterProcessStatus* process_status) const
{
    const fs::path profile_local_path = fs::path(Slic3r::data_dir()) / "profiles" / "local" / "vendor";
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

        std::vector<Index> index_db = load_vendors_db(archive_dir.path());
        for (const auto &index : index_db) {
            const fs::path bundle_path = archive_dir.path() / index.vendor() / (index.vendor() + ".ini");
            ec.clear();

            if (!fs::exists(bundle_path, ec) || ec) {
                process_status->set_warning(Slic3r::GUI::format("File does not exists %1%. %2%", bundle_path.string(), ec.message()));
                continue;
            }

            // Perform a basic load and check the version of the installed preset bundle.
		    VendorProfile vp;
		    try {
			    vp = VendorProfile::from_ini(bundle_path, false);
		    }
		    catch (const std::exception& e) {
			    BOOST_LOG_TRIVIAL(error) << Slic3r::GUI::format("Corrupted profile file for vendor %1% at %2%, message: %3%", index.vendor(), bundle_path, e.what());
			    continue;
		    }
            // Recommended version of vendor
		    const Index::const_iterator recommended = index.recommended();
		    if (recommended == index.end()) {
			    BOOST_LOG_TRIVIAL(error) << Slic3r::GUI::format("No recommended version for vendor: %1%, Index file might be corrupted.", index.vendor());
                assert(false);
			    continue;
		    }
            const Index::const_iterator vendor_current_version_it = index.find(vp.config_version);
		    const bool ver_current_found = vendor_current_version_it != index.end();

		    BOOST_LOG_TRIVIAL(debug) << Slic3r::GUI::format("Vendor: %1%, version installed: %2%%3%, recommended version: %4%",
			    vp.name,
			    vp.config_version.to_string(),
			    (ver_current_found ? "" : " (not found in index!)"),
			    recommended->config_version.to_string());

            if (! ver_current_found) {
			    // Any published config shall be always found in the latest config index.
			    BOOST_LOG_TRIVIAL(error) << Slic3r::GUI::format("Preset bundle `%1%` version not found in index: %2%", index.vendor(), vp.config_version.to_string());
                results.emplace_back(VendorReconfigurationState::VUS_CURRENT_NOT_IN_INDEX, vp.id, archive_id, Slic3r::Semver(), recommended->config_version, recommended->comment, std::string(), std::string());
			    continue;
		    }

            if (vendor_current_version_it->is_current_slic3r_supported()){
                // This slicer needs no forced reconfigurations
                BOOST_LOG_TRIVIAL(error) << Slic3r::GUI::format("Preset bundle `%1%` version %2% needs no forced reconfiguration.",  index.vendor(), vp.config_version);
                continue;
            }

            if (vendor_current_version_it->is_current_slic3r_downgrade()) {
		        BOOST_LOG_TRIVIAL(warning) << "Current Slic3r incompatible with installed bundle (forced downgrade): " << bundle_path.string();
                results.emplace_back(VendorReconfigurationState::VUS_FORCED_DOWNGRADE, vp.id, archive_id, vendor_current_version_it->config_version, recommended->config_version, recommended->comment, std::string(), std::string());
			    continue;
            }
		    BOOST_LOG_TRIVIAL(warning) << "Current Slic3r incompatible with installed bundle (forced update): " << bundle_path.string();
            results.emplace_back(VendorReconfigurationState::VUS_FORCED_UPDATE, vp.id, archive_id, vendor_current_version_it->config_version, recommended->config_version, recommended->comment, std::string(), std::string());
        }
    }
}

void PresetUpdater::check_reconfigurations(ReconfigurationsList& results, PresetUpdaterProcessStatus* process_status) const
{
    BOOST_LOG_TRIVIAL(debug) << __FUNCTION__;

    const fs::path profile_local_path = fs::path(Slic3r::data_dir()) / "profiles" / "local" / "vendor";
    const fs::path update_sync_path = fs::path(Slic3r::data_dir()) / "update_sync";
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

        std::vector<Index> index_db = load_vendors_db(archive_dir.path());
        for (const auto &index : index_db) {
            // the installed bundles are in / "profiles" / "local"
            // the most recent index file should be in / "update_sync"
            // if index in / "profiles" / "local" is more recent use it instead
            // (should the index  in / "profiles" / "local" be rewritten if not most recent?)
            // then check installed bundles version and decide if and what type of reconfiguration
            
            // Compare update_sync index and index
            const fs::path update_sync_index_path = update_sync_path / archive_dir.path().filename() / index.path().filename();
            Index most_recent_index;
            ec.clear();
            if (fs::exists(update_sync_index_path, ec) && !ec) {
                Index update_sync_index;
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
		    VendorProfile vp;
		    try {
			    vp = VendorProfile::from_ini(bundle_path, false);
		    }
		    catch (const std::exception& e) {
			    std::string message = Slic3r::GUI::format("Corrupted profile file for vendor %1% at %2%, message: %3%", index.vendor(), bundle_path, e.what());
                BOOST_LOG_TRIVIAL(error) << message;
                assert(false);
			    continue;
		    }
            // Recommended version of vendor
		    const Index::const_iterator recommended = most_recent_index.recommended();
		    if (recommended == most_recent_index.end()) {
			    BOOST_LOG_TRIVIAL(error) << Slic3r::GUI::format("No recommended version for vendor: %1%, Index file might be corrupted.", index.vendor());
                assert(false);
			    continue;
		    }
            const Index::const_iterator vendor_current_version_it = most_recent_index.find(vp.config_version);
		    const bool current_found = vendor_current_version_it != most_recent_index.end();
            BOOST_LOG_TRIVIAL(debug) << Slic3r::GUI::format("Vendor: %1%, version installed: %2%%3%, recommended version: %4%",
			    vp.name,
			    vp.config_version.to_string(),
			    (current_found ? "" : " (not found in index!)"),
			    recommended->config_version.to_string());
            if (!current_found) {
                BOOST_LOG_TRIVIAL(error) << Slic3r::GUI::format("Current installed version for vendor: %1% (%2%) was not found in index %3%", vp.id, vp.config_version, most_recent_index.path().string());
                // If current is not found in index, we need forced reconfiguration
                results.emplace_back(VendorReconfigurationState::VUS_CURRENT_NOT_IN_INDEX,  vp.id, archive_id, Slic3r::Semver(), recommended->config_version, recommended->comment, std::string(), std::string());
                continue;
            }
            if (vendor_current_version_it->is_current_slic3r_supported()){
                // This slicer needs no forced reconfigurations
                assert (vp.config_version <= recommended->config_version);
                if (vp.config_version < recommended->config_version) {
                    // regular update
                    BOOST_LOG_TRIVIAL(trace) << Slic3r::GUI::format("Preset bundle `%1%` version %2% has update %3%.",  index.vendor(), vp.config_version, recommended->config_version);
                    results.emplace_back(VendorReconfigurationState::VUS_UPDATE,  vp.id, archive_id, vendor_current_version_it->config_version, recommended->config_version, recommended->comment, std::string(), std::string());
                }
                continue;
            }

            if (vendor_current_version_it->is_current_slic3r_downgrade()) {
                // Forced downgrade
		        BOOST_LOG_TRIVIAL(warning) << "Current Slic3r incompatible with installed bundle (forced downgrade): " << bundle_path.string();
                results.emplace_back(VendorReconfigurationState::VUS_FORCED_DOWNGRADE,  vp.id, archive_id, vendor_current_version_it->config_version, recommended->config_version, recommended->comment, std::string(), std::string());
			    continue;
            }
            // Not supported and not downgrade = forced update
            BOOST_LOG_TRIVIAL(warning) << "Current Slic3r incompatible with installed bundle (forced update): " << bundle_path.string();
            results.emplace_back(VendorReconfigurationState::VUS_FORCED_UPDATE,  vp.id, archive_id, vendor_current_version_it->config_version, recommended->config_version, recommended->comment, std::string(), std::string());
        }
    }
}

void PresetUpdater::perform_reconfigurations(const ReconfigurationsList& reconfigurations, PresetUpdaterProcessStatus* process_status) const
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

void PresetUpdater::perform_downgrades(const std::vector<VendorReconfiguration>& downgrades, PresetUpdaterProcessStatus* process_status) const
{
    for (const VendorReconfiguration& downgrade : downgrades) {
        BOOST_LOG_TRIVIAL(info) << "Deleting incompatible bundle " << downgrade.vendor_id; 
        fs::path dir_path = fs::path(Slic3r::data_dir()) / "profiles" / "local" / "vendor" / downgrade.vendor_archive_id / downgrade.vendor_id;
        fs::path idx_path = fs::path(Slic3r::data_dir()) / "profiles" / "local" / "vendor" / downgrade.vendor_archive_id / (downgrade.vendor_id + ".idx");
        boost::system::error_code ec;
        if (!fs::remove_all(dir_path, ec) || ec) {
            process_status->set_error(Slic3r::GUI::format("Failed to remove directory %1% while performing downgrade. %2%",dir_path, ec.message()));
            return;
        }
        if (!fs::remove(idx_path, ec) || ec) {
            process_status->set_error(Slic3r::GUI::format("Failed to remove index %1% while performing downgrade. %2%",idx_path, ec.message()));
            return;
        }
    }
}

namespace {
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
}
void PresetUpdater::perform_updates(const std::vector<VendorReconfiguration>& updates, PresetUpdaterProcessStatus* process_status) const
{
    const fs::path profile_local_path = fs::path(Slic3r::data_dir()) / "profiles" / "local" / "vendor";
    const fs::path update_sync_path = fs::path(Slic3r::data_dir()) / "update_sync";
    boost::system::error_code ec;
    for (const VendorReconfiguration& update : updates) {
        BOOST_LOG_TRIVIAL(info) << "Updating bundle " << update.vendor_id;

        const fs::path vendor_dest_dir_path = profile_local_path / update.vendor_archive_id / update.vendor_id;
        const fs::path vendor_dest_idx_path = profile_local_path / update.vendor_archive_id / (update.vendor_id + ".idx");
        const fs::path vendor_source_dir_path = update_sync_path / update.vendor_archive_id / update.vendor_id;
        const fs::path vendor_source_idx_path = update_sync_path / update.vendor_archive_id / (update.vendor_id + ".idx");

        ec.clear();
        if (!fs::remove_all(vendor_dest_dir_path, ec) || ec) {
            process_status->set_error(Slic3r::GUI::format("Failed to remove directory %1% while performing update. %2%",vendor_dest_dir_path, ec.message()));
            return;
        }
        if (!fs::remove(vendor_dest_idx_path, ec) || ec) {
            process_status->set_error(Slic3r::GUI::format("Failed to remove index %1% while performing update. %2%",vendor_dest_dir_path, ec.message()));
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

} // PresetManagement