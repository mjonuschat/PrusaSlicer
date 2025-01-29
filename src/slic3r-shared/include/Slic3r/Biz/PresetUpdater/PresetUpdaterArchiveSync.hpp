#pragma once

#include <memory>
#include <vector>
#include <map>
#include <string>

namespace boost::filesystem {class path;}

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterIndex;
class PresetUpdaterRepository;

typedef std::vector<const PresetUpdaterRepository*> SharedArchiveRepositoryVector;

class PresetUpdaterProcessStatus;

class PresetUpdaterArchiveSync
{
public:
    PresetUpdaterArchiveSync() = default;
	PresetUpdaterArchiveSync(PresetUpdaterArchiveSync &&) = delete;
	PresetUpdaterArchiveSync(const PresetUpdaterArchiveSync &) = delete;
	PresetUpdaterArchiveSync& operator=(PresetUpdaterArchiveSync &&) = delete;
	PresetUpdaterArchiveSync& operator=(const PresetUpdaterArchiveSync &) = delete;
	~PresetUpdaterArchiveSync() = default;

	void sync(const SharedArchiveRepositoryVector& repositories
		, PresetUpdaterProcessStatus* process_status) const;

private:
    
	void stage_update_sync(const boost::filesystem::path& source_dir
        , const PresetUpdaterRepository* archive
		, PresetUpdaterProcessStatus* process_status) const;

    /// Once there are more then 1 profile file, copy action should first delete all current then copy new ones.
    void copy_bundle_files(const boost::filesystem::path& source, const boost::filesystem::path& target) const;

    /// Once there are more then 1 profile file, copy action should first delete all current then copy new ones.
    void copy_missing_resources(const boost::filesystem::path& source_dir, const boost::filesystem::path& target_dir) const;

    /// Downloads index archive to temp_path. For each index calls sync_not_installed_vendor or sync_installed_vendor.
	void sync_archive(const boost::filesystem::path& temp_path
        , const PresetUpdaterRepository* archive
		, PresetUpdaterProcessStatus* process_status) const;

    /// Downloads recommended bundle to temp_path / vendor_name. Then calls check_missing_resources.   
    void sync_not_installed_vendor(const boost::filesystem::path& temp_path
        , const PresetUpdaterRepository* archive
		, PresetUpdaterProcessStatus* process_status
        , const PresetUpdaterIndex& index) const;

    /// Compares version of installed bundle of vendor with index. Then downloads recommended bundle to temp_path / vendor_name if needed. Then calls check_missing_resources.  
    void sync_installed_vendor(const boost::filesystem::path& temp_path
        , const PresetUpdaterRepository* archive
		, PresetUpdaterProcessStatus* process_status
        , const PresetUpdaterIndex& index) const;

    /// Loads bundle at bundle path and downloads all missing resources to temp_path / vendor_name.
    void check_missing_resources(const boost::filesystem::path& temp_path
        , const PresetUpdaterRepository* archive
		, PresetUpdaterProcessStatus* process_status
        , const boost::filesystem::path& bundle_path) const;

    // TODO
    /*
	void get_missing_resource(const PresetUpdaterRepository* archive
		, const std::string& vendor
		, const std::string& filename
		, const std::string& repository_id_from_ini
		, PresetUpdaterUIStatus* process_status) const;

	void get_or_copy_missing_resource(const PresetUpdaterRepository* archive
		, const std::string& vendor
		, const std::string& filename
		, const std::string& repository_id_from_ini
		, PresetUpdaterUIStatus* process_status) const;

    */
};


} // namespace Slic3r::Biz::PresetUpdater