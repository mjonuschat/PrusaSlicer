#ifndef slic3r_PresetArchiveRepositoryDatabase_hpp_
#define slic3r_PresetArchiveRepositoryDatabase_hpp_

#include "PresetArchiveRepository.hpp"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace PresetManagement {

class PresetUpdaterProcessStatus;

typedef std::vector<std::unique_ptr<PresetArchiveRepository>> PrivateArchiveRepositoryVector;
typedef std::vector<const PresetArchiveRepository*> SharedArchiveRepositoryVector;

class PresetArchiveRepositoryDatabase
{
public:
	PresetArchiveRepositoryDatabase();
    PresetArchiveRepositoryDatabase(PresetArchiveRepositoryDatabase&&) = delete;
    PresetArchiveRepositoryDatabase(const PresetArchiveRepositoryDatabase&) = delete;
    PresetArchiveRepositoryDatabase& operator=(PresetArchiveRepositoryDatabase&&) = delete;
    PresetArchiveRepositoryDatabase& operator=(const PresetArchiveRepositoryDatabase&) = delete; 
	~PresetArchiveRepositoryDatabase() = default;
     
    // returns true if successfully got the data
	bool sync(PresetUpdaterProcessStatus* process_status = nullptr);

	// Do not use get_all_archive_repositories to perform any GET calls. Use get_selected_archive_repositories instead.
    SharedArchiveRepositoryVector get_all_archive_repositories() const;
    // Creates copy of m_archive_repositories of shared pointers that are selected in m_selected_repositories_uuid.
    SharedArchiveRepositoryVector get_selected_archive_repositories() const;
	bool is_selected_repository_by_uuid(const std::string& uuid) const;
	bool is_selected_repository_by_id(const std::string& repo_id) const;
	const std::map<std::string, bool>& get_selected_repositories_uuid() const { assert(m_selected_repositories_uuid.size() == m_archive_repositories.size()); return m_selected_repositories_uuid; }
    // Does re-extract all local archives
	bool set_selected_repositories(const std::vector<std::string>& used_uuids, std::string& msg);
    void set_installed_printer_repositories(const std::vector<std::string> &used_ids);
	std::string add_local_archive(const boost::filesystem::path path, std::string& msg);
	void remove_local_archive(const std::string& uuid);
    bool extract_archives_with_check(std::string &msg);

private:
	void load_app_manifest_json();
	void copy_initial_manifest();
	void read_server_manifest(const std::string& json_body);
	void save_app_manifest_json() const;
	void clear_online_repos();
	bool is_selected(const std::string& uuid) const;
    bool has_installed_printers(const std::string &uuid) const;
	boost::filesystem::path get_stored_manifest_path() const;
	void consolidate_uuid_maps();
    void extract_local_archives();
	std::string get_next_uuid();
	boost::filesystem::path			m_unq_tmp_path;
    PrivateArchiveRepositoryVector  m_archive_repositories;
	std::map<std::string, bool>		m_selected_repositories_uuid;
    std::map<std::string, bool>		m_has_installed_printer_repositories_uuid;
	boost::uuids::random_generator	m_uuid_generator;
};

} // Slic3r

#endif // PresetArchiveRepositoryDatabase