#include "Slic3r/Biz/PresetUpdater/PresetUpdaterArchiveSync.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterProcessStatus.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterRepository.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterUtils.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterVendorProfile.hpp"
#include "Slic3r/Biz/PresetUpdater/PresetUpdaterIndex.hpp"

#include "Slic3r/Exception.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

#include "libslic3r/Utils.hpp"
#include "libslic3r/miniz_extension.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PresetUpdater {

namespace {
void create_temp_dir(fs::path& temp_dir /* = fs::path()*/)
{
    
    boost::uuids::random_generator generator;
	const std::string dirname = boost::uuids::to_string(generator());
    temp_dir = boost::filesystem::temp_directory_path() / dirname;

    boost::system::error_code ec;

    if (fs::exists(temp_dir, ec) && !ec && fs::is_directory(temp_dir, ec) && !ec) {
        SPDLOG_ERROR("Temp directory {} already exists.", temp_dir.string()); // Something is off. This should never happen.
        throw Slic3r::RuntimeError("Failed to create temp directory " + temp_dir.string() + ". " + ec.message());
    }
    ec.clear();

    if (!fs::create_directory(temp_dir, ec)) {
        throw Slic3r::RuntimeError("Failed to create temp directory " + temp_dir.string() + ". " + ec.message());
    }
}

bool is_vendor_installed(const std::string& vendor_id)
{
    // For now lets detect its file in data_dir_path / "profiles" / "local" / "vendor"
    // Later this function should rather detect vendor presence in app runtime data
    fs::path installed_vendors_dir = fs::path(Slic3r::data_dir()) / "profiles" / "local" / "vendor";
    boost::system::error_code ec;
    if (!fs::exists(installed_vendors_dir, ec) || ec) {
        throw Slic3r::RuntimeError(installed_vendors_dir.string() + " does not exists. " + ec.message());
        return false;
    }
    if (!fs::is_directory(installed_vendors_dir, ec) || ec) {
        throw Slic3r::RuntimeError(installed_vendors_dir.string() + " is not a directory. " + ec.message());
        return false;
    }
    fs::path vendor_folder_path = installed_vendors_dir / vendor_id;
    if (!fs::exists(vendor_folder_path, ec) || ec) {
        return false;
    }
    if (!fs::is_directory(vendor_folder_path, ec) || ec) {
        return false;
    }
    // lets take presence of index file as proof of installation (at time vendor file structure is not set)
    fs::path vendor_index_path = vendor_folder_path / (vendor_id + ".idx");
    if (!fs::exists(vendor_index_path, ec) || ec) {
        return false;
    }
    return true;
}
} // namespace

void PresetUpdaterArchiveSync::sync(
	const SharedArchiveRepositoryVector& repositories,
	PresetUpdaterProcessStatus* process_status) const
{
    ASSERT(process_status);
    const fs::path resources_dir = fs::path(Slic3r::resources_dir()) / "profiles";
    for (const PresetUpdaterRepository* archive : repositories) {
        // Even if process_status says user canceled, we need stage_update_sync_dir.
        // But we need to end if thread itself is canceling.
        if (process_status->get_force_canceled()) {
            return;
        }
        stage_update_sync(resources_dir / archive->get_manifest().id, archive, process_status);

	}
    // Create workspace in OS temp folder.
    fs::path temp_dir;
    try
    {
        create_temp_dir(temp_dir);
    }
    catch (const  Slic3r::RuntimeError& e)
    {
        process_status->set_error(std::string("Preset Archive Sync has failed. ") + e.what());
        return;
    }

    // perform sync on every repository
    for (const PresetUpdaterRepository* archive : repositories) {
        if (process_status->get_canceled()) {
            break;
        }
	    this->sync_archive(temp_dir / archive->get_manifest().id, archive, process_status);
	}
    
    for (const PresetUpdaterRepository* archive : repositories) {
        // Even if process_status says user canceled, we need stage_update_sync_dir.
        // But we need to end if thread itself is canceling.
        if (process_status->get_force_canceled()) {
            return;
        }
        stage_update_sync(temp_dir / archive->get_manifest().id, archive, process_status);
	}
}

void PresetUpdaterArchiveSync::stage_update_sync(
    const boost::filesystem::path& source_dir,
    const PresetUpdaterRepository* archive, 
    PresetUpdaterProcessStatus* process_status) const
{
    
    // Copy index and bundle over existing?
    // OR
    // Compare existing and resources index?
    // Probably compare.
    // Then copy rest of the files (resources) if missing
    
    //SPDLOG_INFO(__FUNCTION__);

    boost::system::error_code ec;
    // Each archive has its own subdir.
    if(!fs::exists(source_dir) || ec) {
        process_status->set_warning("Directory does not exists " + source_dir.string() + ". " + ec.message());
        return;
    }
    
    const fs::path target_archive_dir = fs::path(Slic3r::data_dir()) / "update_sync" / archive->get_manifest().id;
    ec.clear();
    if(!fs::exists(target_archive_dir) || ec) {
        ec.clear();
        if (!fs::create_directory(target_archive_dir, ec)) {
             process_status->set_warning("Failed to create archive dir " + target_archive_dir.string() + ". " + ec.message());
             return;
        }
    }

    std::vector<PresetUpdaterIndex> index_db = load_vendors_db(source_dir);
    for (const PresetUpdaterIndex& source_index: index_db) {

        const fs::path target_vendor_dir = fs::path(Slic3r::data_dir()) / "update_sync" / archive->get_manifest().id / source_index.vendor();
        const fs::path target_index_path = fs::path(Slic3r::data_dir()) / "update_sync" / archive->get_manifest().id / (source_index.vendor() + ".idx"); // index is outside vendor folder.
        const fs::path target_bundle = target_vendor_dir / (source_index.vendor() + ".ini");
        const fs::path target_resources = target_vendor_dir / "resources";

        const fs::path source_vendor_dir = source_dir / source_index.vendor();
        const fs::path source_bundle = source_vendor_dir / (source_index.vendor() + ".ini");
        const fs::path source_resources = source_vendor_dir / "resources";

         if(!fs::exists(target_vendor_dir) || ec) {
            ec.clear();
            if (!fs::create_directory(target_vendor_dir, ec)) {
                process_status->set_warning("Failed to create vendor dir " + target_archive_dir.string() + ". " + ec.message());
                return;
            }
        }
        // Cases:
        // 0) Source is incomplete - error
        // 1) Target is empty - just copy source
        // 2) Target index is higher than source index, then target bundle should be recommended
        // 3) Target index is same as source index - do nothing
        // 4) Target index is lower than source index - if target bundle not recommended, copy from source
        // Then check if all resources in source are in target resources

        
        // 0) Perform a basic load and check the version of the source preset bundle.
        PresetUpdaterVendorProfile source_vp;
        try {
    	    source_vp = PresetUpdaterVendorProfile::from_ini(source_bundle, false);
        }
        catch (const std::exception& e) {
    	    SPDLOG_ERROR("Corrupted profile file for vendor {} at {}, message: {}", source_index.vendor(), source_bundle.string(), e.what());
    	    continue;
        }
        // Recommended version of vendor
        const PresetUpdaterIndex::const_iterator source_recommended = source_index.recommended();
        if (source_recommended == source_index.end()) {
            process_status->set_warning(format("No recommended version for vendor: %1%, Index file might be corrupted. %2%", source_index.vendor(), source_index.path().string()));
    	    continue;
        }
        const PresetUpdaterIndex::const_iterator source_version_it = source_index.find(source_vp.config_version);
        const bool source_version_found = source_version_it != source_index.end(); 
        if ( !source_version_found && source_version_it != source_recommended) {
            process_status->set_warning(format("Not matching bundle version and index %1%.", source_bundle.string()));
            continue;
        }
        
        // 1) Check existence of target index
        ec.clear();
        if (!fs::exists(target_index_path, ec) || ec) {
            copy_file_fix(source_index.path(), target_index_path);
            copy_bundle_files(source_bundle, target_bundle);
            copy_missing_resources(source_resources, target_resources);
            continue;
        }
        
        // 2) Target index is higher than source index, copy just resources
        PresetUpdaterIndex target_index;
        target_index.load(target_index_path);
        if (target_index.version() > source_index.version()) {
            copy_missing_resources(source_resources, target_resources);
            continue;
        }

        // 3) Target index is same as source index - copy just resources
        if (target_index.version() == source_index.version()) {
            copy_missing_resources(source_resources, target_resources);
            continue;
        }

        // 3) Target index is lower than source index - if target bundle not recommended in SOURCE, copy from source
        if (target_index.version() < source_index.version()) {
             PresetUpdaterVendorProfile target_vp;
            try {
    	        target_vp = PresetUpdaterVendorProfile::from_ini(target_bundle, false);
            }
            catch (const std::exception& e) {
    	        SPDLOG_ERROR("Corrupted profile file for vendor {} at {}, message: {}", target_index.vendor(), target_bundle.string(), e.what());
    	        continue;
            }
            const PresetUpdaterIndex::const_iterator target_version_it = source_index.find(target_vp.config_version);
            const bool target_version_found = target_version_it != source_index.end(); 
            if ( !target_version_found && target_version_it != source_recommended) {
                copy_bundle_files(source_bundle, target_bundle);
                continue;
            }
            copy_file_fix(source_index.path(), target_index_path);
            copy_missing_resources(source_resources, target_resources);
            continue;
        }
    }
}

void PresetUpdaterArchiveSync::copy_bundle_files(const boost::filesystem::path& source, const boost::filesystem::path& target) const 
{
    // TODO: remove recursively once there are more yaml files
    boost::system::error_code ec;
    if (fs::exists(target, ec) && !ec) {
        ec.clear();
        if (!fs::remove(target, ec) || ec) {
            SPDLOG_ERROR("Failed to remove file {}", target.string());
            assert(false);
        }
    }
    copy_file_fix(source, target);
}

void PresetUpdaterArchiveSync::copy_missing_resources(const boost::filesystem::path& source_dir, const boost::filesystem::path& target_dir) const
{
    boost::system::error_code ec;
    if (!fs::exists(source_dir, ec)  || ec) {
        // There t not be any resources.
        return;
    }
    if (!fs::exists(target_dir, ec)  || ec) {
        ec.clear();
        if (!fs::create_directory(target_dir, ec)) {
             SPDLOG_ERROR("Failed to create resources dir {}", target_dir.string());
             return;
        }
    }
    fs::directory_iterator source_directory_iterator(source_dir);
    for (auto &dir_entry : source_directory_iterator) {
        ec.clear();
        const fs::path target = target_dir / dir_entry.path().filename();
        if (!fs::exists(target, ec)  || ec) {
            copy_file_fix(dir_entry.path(), target);
        }
    }
}

void PresetUpdaterArchiveSync::sync_archive(
    const boost::filesystem::path& temp_dir,
	const PresetUpdaterRepository* archive_repository,
	PresetUpdaterProcessStatus* process_status) const
{
    //SPDLOG_INFO(__FUNCTION__);
    process_status->set_target(archive_repository->get_manifest().id + " archive");

    boost::system::error_code ec;

    // Each archive has its own subdir.
    assert(!fs::exists(temp_dir) || ec);
    ec.clear();
    if (!fs::create_directory(temp_dir, ec)) {
        throw Slic3r::RuntimeError("Failed to create temp directory " + temp_dir.string() + ". " + ec.message());
        return;
    }
    
    
	fs::path archive_path(temp_dir / "vendor_indices.zip");
    assert(!fs::exists(archive_path, ec) || ec);
    // Download profiles archive zip
	if (!archive_repository->get_archive(archive_path, process_status)) {
		SPDLOG_ERROR("Download of vendor profiles archive zip of {} repository has failed.", archive_repository->get_manifest().id);
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
    //    |- data # all data needed
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
		SPDLOG_ERROR("Couldn't open zipped bundle. Sync config has failed for repository {}", archive_repository->get_manifest().id);
		return;
	} else {
		mz_uint num_entries = mz_zip_reader_get_num_files(&archive);
		// loop the entries 
		mz_zip_archive_file_stat stat;
		for (mz_uint i = 0; i < num_entries; ++i) {
			if (mz_zip_reader_file_stat(&archive, i, &stat)) {
				std::string name(stat.m_filename);
				if (stat.m_uncomp_size > 0) {
					std::string buffer((size_t)stat.m_uncomp_size, 0);
					mz_bool res = mz_zip_reader_extract_to_mem(&archive, stat.m_file_index, (void*)buffer.data(), (size_t)stat.m_uncomp_size, 0);
					if (res == 0) {
						SPDLOG_ERROR("Failed to unzip {}", stat.m_filename);
						continue;
					}
					// create file from buffer
					fs::path tmp_path(temp_dir / (name + ".tmp"));
					if (!fs::exists(tmp_path.parent_path())) {
						SPDLOG_ERROR("Failed to unzip file {}. Directories are not supported. Skipping file.", name);
						continue;
					}
					fs::path target_path(temp_dir / name);
					boost::nowide::fstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
					file.write(buffer.c_str(), buffer.size());
					file.close();
					boost::system::error_code ec;
					bool exists = fs::exists(tmp_path, ec);
					if(!exists || ec) {
						SPDLOG_ERROR("Failed to find unzipped file at {}. Terminating Preset updater synchronization.", tmp_path.string());
						Slic3r::close_zip_reader(&archive);
						return;
					}
					fs::rename(tmp_path, target_path, ec);
					if (ec) {
						SPDLOG_ERROR("Failed to rename unzipped file at {}. Terminating Preset updater synchorinzation. Error message: {}", tmp_path.string(), ec.message());
						Slic3r::close_zip_reader(&archive);
						return;
					}
					// TODO: what if unexpected happens here (folder inside zip) - crash! 

					if (name.substr(name.size() - 3) == "idx") {
                        // TODO: why it was substracted
						//vendors_list.emplace_back(name.substr(0, name.size() - 4));
                        vendors_list.emplace_back(name);
                    }
				}
			}
		}
		Slic3r::close_zip_reader(&archive);
	}
    
    // Now we have vendors_list, but we need index_db.
    std::vector<PresetUpdaterIndex> index_db = load_vendors_db_filtered(temp_dir, vendors_list);

    for (const auto& index: index_db) {
        if (process_status->get_canceled()) { 
			return; 
		}
        try
        {
            if (!is_vendor_installed(index.vendor())) {
                // For not installed vendor we need to download its recommended version.
                sync_not_installed_vendor(temp_dir, archive_repository, process_status, index);
                continue;
            }
        }
        catch (const Slic3r::RuntimeError& e)
        {
            process_status->set_warning(e.what());
            continue;
        }
        
        // if installed, it can still mean there is different recommended version.
        sync_installed_vendor(temp_dir, archive_repository, process_status, index);
    }
}

void PresetUpdaterArchiveSync::sync_not_installed_vendor(
    const boost::filesystem::path& temp_path,
    const PresetUpdaterRepository* archive,
	PresetUpdaterProcessStatus* process_status,
    const PresetUpdaterIndex& index) const
{
    // First download recommended version of profiles.
    // It could be same as in resources, still download it.

    const fs::path target_bundle_path = temp_path / index.vendor() / (index.vendor() + ".ini");
    boost::system::error_code ec;
    assert (fs::exists(index.path(), ec) && !ec);

    // Recommended version of vendor
    const PresetUpdaterIndex::const_iterator recommended = index.recommended();
    if (recommended == index.end()) {
    	SPDLOG_ERROR("No recommended version for vendor: {}, Index file might be corrupted.", index.vendor());
        assert(false);
    	return;
    }

    // Download recommended version.
    ec.clear();
    if (!fs::exists(target_bundle_path.parent_path(), ec) || ec) {
        ec.clear();
        if (!fs::create_directory(target_bundle_path.parent_path(), ec)) {
            SPDLOG_ERROR("Failed to create target directory {} for vendor {}", target_bundle_path.parent_path().string(), index.vendor());
            return;
        }
    }
    const std::string source_subpath = format("%1%/%2%.ini", index.vendor(), recommended->config_version.to_string());
    if (!archive->get_ini_no_id(source_subpath, target_bundle_path, process_status)) {
        return;
    }

    // Use downloaded bundle for check. 
    check_missing_resources(temp_path, archive, process_status, target_bundle_path);
}
void PresetUpdaterArchiveSync::sync_installed_vendor(
    const boost::filesystem::path& temp_path,
    const PresetUpdaterRepository* archive,
	PresetUpdaterProcessStatus* process_status,
    const PresetUpdaterIndex& index) const
{
    const fs::path profile_local_dir_path = fs::path(Slic3r::data_dir()) / "profiles" / "local" / "vendor";
    const fs::path installed_bundle_path = profile_local_dir_path / (index.vendor() + ".ini");
    /* not yet - download to temp
    const fs::path update_sync_dir_path = fs::path(Slic3r::data_dir()) / "update_sync";
    const fs::path target_bundle_path = update_sync_dir_path / (index.vendor() + ".ini");
    */
    const fs::path target_bundle_path = temp_path / index.vendor() / (index.vendor() + ".ini");
    boost::system::error_code ec;
    assert (fs::exists(index.path(), ec) && !ec);
    assert (fs::exists(installed_bundle_path, ec) && !ec);
    
    // Perform a basic load and check the version of the installed preset bundle.
    PresetUpdaterVendorProfile vp;
    try {
    	vp = PresetUpdaterVendorProfile::from_ini(installed_bundle_path, false);
    }
    catch (const std::exception& e) {
    	SPDLOG_ERROR("Corrupted profile file for vendor {} at {}, message: {}", index.vendor(), installed_bundle_path.string(), e.what());
    	return;
    }
    // Recommended version of vendor
    const PresetUpdaterIndex::const_iterator recommended = index.recommended();
    if (recommended == index.end()) {
    	SPDLOG_ERROR("No recommended version for vendor: {}, Index file might be corrupted.", index.vendor());
        assert(false);
    	return;
    }
    const PresetUpdaterIndex::const_iterator vendor_current_version_it = index.find(vp.config_version);
    const bool ver_current_found = vendor_current_version_it != index.end();
    
    // Commented is a part that skips downloading profiles when installed is recommended.
    /*
    if ( ver_current_found && vendor_current_version_it == recommended) {
    	// No need to download profiles - recommended are installed
        BOOST_LOG_TRIVIAL(trace) << index.vendor() << ": most recent index confirms installed profiles are recommended version.";
        // Use installed profiles for check. 
        check_missing_resources(temp_path, archive, process_status, installed_bundle_path);
        return;
    }
    */

    // Download recommended version.
    ec.clear();
    if (!fs::exists(target_bundle_path.parent_path(), ec) || ec) {
        ec.clear();
        if (!fs::create_directory(target_bundle_path.parent_path(), ec)) {
            SPDLOG_ERROR("Failed to create target directory {} for vendor {}", target_bundle_path.parent_path().string(), index.vendor());
            return;
        }
    }
    const std::string source_subpath = format("%1%/%2%.ini", index.vendor(), recommended->config_version.to_string());
    if (!archive->get_file(source_subpath, target_bundle_path, vp.repo_id, process_status)) {
        return;
    }
    // Use downloaded bundle for check. 
    check_missing_resources(temp_path, archive, process_status, target_bundle_path);
}

void PresetUpdaterArchiveSync::check_missing_resources(
    const boost::filesystem::path& temp_path,
    const PresetUpdaterRepository* archive,
	PresetUpdaterProcessStatus* process_status,
    const boost::filesystem::path& bundle_path) const
{
    // Load bundle to read list of resources.
    PresetUpdaterVendorProfile vp;
    try {
    	vp = PresetUpdaterVendorProfile::from_ini(bundle_path, true);
    }
    catch (const std::exception& e) {
    	SPDLOG_ERROR("Corrupted profile file {}, message: {}", bundle_path.string(), e.what());
    	return;
    }

    /*
    for (const auto& model : vp.models) {
    	if (!model.thumbnail.empty()) {
    		try
    		{
    			get_missing_resource(archive_repository, vp.id, model.thumbnail, vp.repo_id, process_status);
    		}
    		catch (const std::exception& e)
    		{
    			BOOST_LOG_TRIVIAL(error) << "Failed to get " << model.thumbnail << " for " << vp.id << " " << model.id << ": " << e.what();
    		}
    	}
    	if (process_status->get_canceled())
    		return;
    }
    */
}

/*
void PresetUpdaterArchiveSync::get_missing_resource(
	const PresetUpdaterRepository* archive,
	const std::string& vendor,
	const std::string& filename,
	const std::string& repository_id_from_ini,
	PresetUpdaterUIStatus* process_status) const
{

}

void PresetUpdaterArchiveSync::get_or_copy_missing_resource(
	const PresetUpdaterRepository* archive,
	const std::string& vendor,
	const std::string& filename,
	const std::string& repository_id_from_ini,
	PresetUpdaterUIStatus* process_status) const
{
}
*/ 
} // namespace Slic3r::Biz::PresetUpdater