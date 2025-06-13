#pragma once

#include "Slic3r/Biz/PresetUpdater/PresetUpdaterRepository.hpp"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterProcessStatus;

typedef std::vector<std::unique_ptr<AbstractPresetUpdaterRepository>> PrivateRepositoryVector;
typedef std::vector<const AbstractPresetUpdaterRepository*> SharedRepositoryVector;

class PresetUpdaterRepositoryDatabase
{
public:
    /**
     * @brief Constructor does loads manifest in datadir.
     */
	PresetUpdaterRepositoryDatabase(PresetUpdaterProcessStatus* process_status);
    PresetUpdaterRepositoryDatabase(PresetUpdaterRepositoryDatabase&&) = delete;
    PresetUpdaterRepositoryDatabase(const PresetUpdaterRepositoryDatabase&) = delete;
    PresetUpdaterRepositoryDatabase& operator=(PresetUpdaterRepositoryDatabase&&) = delete;
    PresetUpdaterRepositoryDatabase& operator=(const PresetUpdaterRepositoryDatabase&) = delete; 
	~PresetUpdaterRepositoryDatabase();
     
    /**
     * @brief Performs GET for online repo manifest. Then loads it, merges with current data (loaded in constructor) and stores into manifest in datadir.
     * Returns true if successfully got the data.
     */
	bool sync(PresetUpdaterProcessStatus* process_status);

	/**
     * @brief Returns Vector with all repository descriptor, uuid and selected flag. 
     * Its all Frontend needs to show repositories to user. 
     */
    SharedPresetUpdaterRepositoryInfoVector get_all_repositories() const;

    /**
     * @brief Returns const pointers to selected repositories. Used by prepare update_sync and perform reconfigurations 
     */
    SharedRepositoryVector get_selected_repositories() const;

    /**
     * @brief Changes "selected" flag on repositories. 
     */
    void apply_selection(const SharedPresetUpdaterRepositoryInfoVector& repos, PresetUpdaterProcessStatus* process_status);

    /**
     * @brief Creates new local repository, unzips it to its newly created directory.
     * Unzipped data lives in datadir until removal of local repo.
     */
    void add_local_repository(const boost::filesystem::path& zip_path, PresetUpdaterProcessStatus* process_status);

    /**
     * @brief Removes local repository, deletes its data directory. 
     */
	void remove_local_repository(const std::string& uuid, PresetUpdaterProcessStatus* process_status);

private:
    /// @brief Reads manifest file in data dir or creates it from resources. Called from constructor. Fills repository vectors and maps. 
	void load_app_manifest_json(PresetUpdaterProcessStatus* process_status);

    /// @brief Updates repositories over manifest that was downloaded in sync(). Called from sync().
    void read_server_manifest(const std::string& json_body, PresetUpdaterProcessStatus* process_status);

    /// @brief Rewrites manifest file in datadir with current repos.
    void save_app_manifest_json(PresetUpdaterProcessStatus* process_status) const;

    // Helper methods
	void copy_initial_manifest() const;
    void clear_online_repos();
	std::string get_next_uuid();
    boost::filesystem::path get_stored_manifest_path() const;
		

	boost::filesystem::path			m_unq_tmp_path;
    PrivateRepositoryVector  m_all_repositories;
	boost::uuids::random_generator	m_uuid_generator;
};

} // namespace Slic3r::Biz::PresetUpdater