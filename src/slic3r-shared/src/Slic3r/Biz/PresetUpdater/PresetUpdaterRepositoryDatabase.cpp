#include "Slic3r/Biz/PresetUpdater/PresetUpdaterRepositoryDatabase.hpp"

#include "Slic3r/Biz/PresetUpdater/PresetUpdaterProcessStatus.hpp"
#include "Slic3r/Biz/Network/IHttp.hpp"
#include "Slic3r/Biz/Network/ServiceConfig.hpp"
#include "Slic3r/Biz/CopyFile.hpp"
#include "Slic3r/Biz/Directories.hpp"

#include "Slic3r/Exception.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

#include <boost/filesystem/directory.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "nlohmann/json.hpp"
#include <fmt/format.h>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PresetUpdater {

PresetUpdaterRepositoryDatabase::PresetUpdaterRepositoryDatabase()
{
    boost::system::error_code ec;
	m_unq_tmp_path = fs::temp_directory_path() / fs::unique_path();
	fs::create_directories(m_unq_tmp_path, ec);
	assert(!ec);
    try
    {
        load_app_manifest_json();
    }
    catch (const Slic3r::RuntimeError& e)
    {
        SPDLOG_ERROR("Failed to load archive repository manifest: {}", e.what());
    }
	
}

bool PresetUpdaterRepositoryDatabase::set_selected_repositories(const std::vector<std::string>& selected_uuids, std::string& msg)
{
    // First re-extract locals, this will set is_extracted flag
    extract_local_archives();
	// Check if some uuids leads to the same id (online vs local conflict)
	std::map<std::string, std::string> used_set;
	for (const std::string& uuid : selected_uuids) {
		std::string id;
		std::string name;
		for (const auto& archive : m_archive_repositories) {
			if (archive->get_uuid() != uuid) {
                continue;
            }        
		    id = archive->get_manifest().id;
		    name = archive->get_manifest().name;
            if (!archive->is_extracted()) {
                // non existent local repo since start selected
                msg = fmt::format(
                    "Cannot select local source from path: {}. It was not extracted.",
                    archive->get_manifest().source_path.string()
                );
                return false;
            }
		    break;
		}
		assert(!id.empty());
		if (auto it = used_set.find(id); it != used_set.end()) {
			msg = fmt::format("Cannot select two sources with the same id: {} and {}", it->second, name);
			return false;
		}
		used_set.emplace(id, name);
	}
	// deselect all first
	for (auto& pair : m_selected_repositories_uuid) {
		pair.second = false;
	}
	for (const std::string& uuid : selected_uuids) {
		m_selected_repositories_uuid[uuid] = true;
	}
    try
    {
        save_app_manifest_json();
    }
    catch (const Slic3r::RuntimeError& e)
    {
        msg = fmt::format("Failed to save app manifest: {}", e.what());
        return false;
    }	
	return true;
}

bool PresetUpdaterRepositoryDatabase::extract_archives_with_check(std::string &msg)
{
    extract_local_archives();
    // std::map<std::string, bool> m_selected_repositories_uuid
    for (const auto& pair : m_selected_repositories_uuid) {
        if (!pair.second) {
            continue;
        }
        const std::string uuid = pair.first;
        auto compare_repo = [&uuid](const std::unique_ptr<PresetUpdaterRepository> &repo) {
            return repo->get_uuid() == uuid;
        };

        const auto& archives_it =std::find_if(m_archive_repositories.begin(), m_archive_repositories.end(), compare_repo);
        assert(archives_it != m_archive_repositories.end());
        if (!archives_it->get()->is_extracted()) {
            // non existent local repo since start selected
            msg += std::string(msg.empty() ? "" : "\n") + archives_it->get()->get_manifest().source_path.string();
        }
    }
    return msg.empty();
}

void PresetUpdaterRepositoryDatabase::set_installed_printer_repositories(const std::vector<std::string> &used_ids)
{
	// set all uuids as not having installed printer
    m_has_installed_printer_repositories_uuid.clear();
    for (const auto &archive : m_archive_repositories) {
        m_has_installed_printer_repositories_uuid.emplace(archive->get_uuid(), false);
	}
	// set correct repos as having installed printer
    for (const std::string &used_id : used_ids) {
		// find archive with id and is used
        std::vector<std::string> selected_uuid;
        std::vector<std::string> unselected_uuid;
        for (const auto &archive : m_archive_repositories) {
            if (archive->get_manifest().id != used_id) {
				continue;
			}	
			const std::string uuid = archive->get_uuid();
            if (m_selected_repositories_uuid[uuid]) {
                selected_uuid.emplace_back(uuid);
            } else {
                unselected_uuid.emplace_back(uuid);
            }
		}
        
        if (selected_uuid.empty() && unselected_uuid.empty()) {
            // there is id in used_ids that is not in m_archive_repositories - BAD
            assert(false);
            continue;
        } else if (selected_uuid.size() == 1){
            // regular case
             m_has_installed_printer_repositories_uuid[selected_uuid.front()] = true;
        } else if (selected_uuid.size() > 1) {
            // this should not happen, only one repo of same id should be selected (online / local conflict)
            assert(false);
            // select first one to solve the conflict
            m_has_installed_printer_repositories_uuid[selected_uuid.front()] = true;
            // unselect the rest
            for (size_t i = 1; i < selected_uuid.size(); i++) {
                m_selected_repositories_uuid[selected_uuid[i]] = false;
            }
        } else if (selected_uuid.empty()) {
            // This is a rare case, where there are no selected repos with matching id but id has installed printers
            // Repro: install printer, unselect repo in the next run of wizard, next, cancel wizard, run wizard again and press finish.
            // Solution: Select the first unselected 
            m_has_installed_printer_repositories_uuid[unselected_uuid.front()] = true;
            m_selected_repositories_uuid[unselected_uuid.front()] = true;
        }

	}
    try
    {
        save_app_manifest_json();
    }
    catch (const Slic3r::RuntimeError& e)
    {
        SPDLOG_ERROR("Failed to save app manifest: {}", e.what());
    }	
}

std::string PresetUpdaterRepositoryDatabase::add_local_archive(const boost::filesystem::path path, std::string& msg)
{
	if (auto it = std::find_if(m_archive_repositories.begin(), m_archive_repositories.end(), [path](const std::unique_ptr<PresetUpdaterRepository>& ptr) {
		return ptr->get_manifest().source_path == path;
		}); it != m_archive_repositories.end())
	{
		msg = fmt::format("Failed to add local archive {}. Path already used.", path.string());
		SPDLOG_ERROR(msg);
		return std::string();
	}
	std::string uuid = get_next_uuid();
	PresetUpdaterRepository::RepositoryManifest header_data;
    header_data.source_path = path;
    header_data.tmp_path = m_unq_tmp_path / uuid;
	if (!PresetUpdaterRepositoryLocal::extract_local_archive_repository(header_data)) {
		msg = fmt::format("Failed to extract local archive {}.", path.string());
		SPDLOG_ERROR(msg);
		return std::string();
	}
	// Solve if it can be set true first.
	m_selected_repositories_uuid[uuid] = false;
    m_has_installed_printer_repositories_uuid[uuid] = false;
	m_archive_repositories.emplace_back(std::make_unique<PresetUpdaterRepositoryLocal>(uuid, std::move(header_data), true));

	try
    {
        save_app_manifest_json();
    }
    catch (const Slic3r::RuntimeError& e)
    {
        msg = fmt::format("Failed to save app manifest: %1%", e.what());
        return std::string();
    }
	return uuid;
}
void PresetUpdaterRepositoryDatabase::remove_local_archive(const std::string& uuid)
{
	auto compare_repo = [uuid](const std::unique_ptr<PresetUpdaterRepository>& repo) {
		return repo->get_uuid() == uuid;
	};

	auto archives_it = std::find_if(m_archive_repositories.begin(), m_archive_repositories.end(), compare_repo);
    if (archives_it == m_archive_repositories.end()) {
        return;
    }
    if (archives_it->get()->get_manifest().source_path.empty()) {
        SPDLOG_ERROR("Attempting to remove archive repository that is not local! Removing local archive repository is canceled.");
        return;
    }
	std::string removed_uuid = archives_it->get()->get_uuid();
	m_archive_repositories.erase(archives_it);
	
	auto used_it = m_selected_repositories_uuid.find(removed_uuid);
	assert(used_it != m_selected_repositories_uuid.end());
	m_selected_repositories_uuid.erase(used_it);

    auto inst_it = m_has_installed_printer_repositories_uuid.find(removed_uuid);
    assert(inst_it != m_has_installed_printer_repositories_uuid.end());
    m_has_installed_printer_repositories_uuid.erase(inst_it);

	try
    {
        save_app_manifest_json();
    }
    catch (const Slic3r::RuntimeError& e)
    {
        SPDLOG_ERROR("Failed to save app manifest: {}", e.what());
    }
}

 void PresetUpdaterRepositoryDatabase::extract_local_archives()
 {
    for (auto &archive : m_archive_repositories) {
         archive->do_extract();
    }
 }    

void PresetUpdaterRepositoryDatabase::load_app_manifest_json()
{
	const fs::path path = get_stored_manifest_path();
    boost::system::error_code ec;
	if (!fs::exists(path, ec) || ec) {
        copy_initial_manifest();
	}
	boost::nowide::ifstream file(path.string());
	std::string data;
	if (file.is_open()) {
		std::string line;
		while (getline(file, line)) {
			data += line;
		}
		file.close();
	}
	else {
		assert(false);
		SPDLOG_ERROR("Failed to read Archive Source Manifest at {}", path.string());
	}
	if (data.empty()) {
		return;
	}

	m_archive_repositories.clear();
    m_selected_repositories_uuid.clear();
    m_has_installed_printer_repositories_uuid.clear();

    try
    {
	    nlohmann::json j = nlohmann::json::parse(data, nullptr, false);

	    if (j.is_discarded() || !j.is_array()) {
		    SPDLOG_ERROR("Failed to parse archives JSON: Input is not a valid JSON array.");
		    return;
	    }

	    for (const auto& repo_json : j) {
            std::string uuid = get_next_uuid();

		    // if "source_path" exists, it's a local repo, else it's an online repo
		    if (repo_json.contains("source_path")) {
			    PresetUpdaterRepository::RepositoryManifest manifest;
			    manifest.source_path = repo_json.at("source_path").get<std::string>();
			    manifest.tmp_path    = m_unq_tmp_path / uuid;

			    bool extracted = PresetUpdaterRepositoryLocal::extract_local_archive_repository(manifest);
			    bool selected     = repo_json.value("selected", true);
			    bool has_printers = repo_json.value("has_installed_printers", false);

			    m_selected_repositories_uuid[uuid]              = extracted && selected;
			    m_has_installed_printer_repositories_uuid[uuid] = extracted && has_printers;

			    m_archive_repositories.emplace_back(std::make_unique<PresetUpdaterRepositoryLocal>(std::move(uuid), std::move(manifest), extracted));			
		    } else {
			    // Online repo
			    PresetUpdaterRepository::RepositoryManifest manifest;
			    if (!PresetUpdaterRepository::extract_repository_header(repo_json, manifest)) {
				    SPDLOG_ERROR("Failed to read header for an online source repository.");
				    continue;
			    }

			    m_selected_repositories_uuid[uuid]              = repo_json.value("selected", true);
			    m_has_installed_printer_repositories_uuid[uuid] = repo_json.value("has_installed_printers", false);

			    m_archive_repositories.emplace_back(std::make_unique<PresetUpdaterRepositoryOnline>(std::move(uuid), std::move(manifest)));
		    }
	    }
    }
    catch (const std::exception& e)
    {
	    SPDLOG_ERROR("Failed to read archives JSON. Reason: {}", e.what());
    }
}

void PresetUpdaterRepositoryDatabase::copy_initial_manifest()
{
	const fs::path target_path = get_stored_manifest_path();
	const fs::path source_path = fs::path(Utils::resources_dir()) / "profiles" / "ArchiveRepositoryManifest.json";
    boost::system::error_code ec;
    if (!fs::exists(source_path, ec) || ec) {
        throw Slic3r::RuntimeError(source_path.string() + " does not exists. " + ec.message());
    }
	std::string error_message;
	Utils::CopyFileResult cfr = Utils::copy_file(source_path.string(), target_path.string(), error_message, false);
	if (cfr != Utils::CopyFileResult::Success) {
        throw Slic3r::RuntimeError("Failed to copy ArchiveRepositoryManifest.json from resources.");
	}
	static constexpr const auto perms = fs::owner_read | fs::owner_write | fs::group_read | fs::others_read;
	fs::permissions(target_path, perms);
}

void PresetUpdaterRepositoryDatabase::save_app_manifest_json() const
{
	/*
	[{
		"name": "Production",
		"description": "Production repository",
		"visibility": null,
		"id": "prod",
		"url": "http://10.24.3.3:8001/v1/repos/prod",
		"index_url": "http://10.24.3.3:8001/v1/repos/prod/vendor_indices.zip"
        "selected": 1
		"has_installed_printers": 1
	}, {
		"name": "Development",
		"description": "Production repository",
        "visibility": "developers only",
		"id": "dev",
		"url": "http://10.24.3.3:8001/v1/repos/dev",
		"index_url": "http://10.24.3.3:8001/v1/repos/dev/vendor_indices.zip"
        "selected": 0
        "has_installed_printers": 0
	}]
	*/
	std::string data = "[";

	for (const auto& archive : m_archive_repositories) {
		// local writes only source_path and "selected". Rest is read from zip on source_path.
		if (!archive->get_manifest().tmp_path.empty()) {
			const PresetUpdaterRepository::RepositoryManifest& man = archive->get_manifest();
			std::string line = archive == m_archive_repositories.front() ? std::string() : ",";
			line += fmt::format(
				"{{"
				"\"source_path\": \"{}\","
				"\"selected\": {},"
				"\"has_installed_printers\": {}"
				"}}",
                man.source_path.generic_string()
				, is_selected(archive->get_uuid()) ? "1" : "0"
                , has_installed_printers(archive->get_uuid()) ? "1" : "0"
            );
			data += line;
			continue;
		}
		// online repo writes whole manifest - in case of offline run, this info is load from here
		const PresetUpdaterRepository::RepositoryManifest& man = archive->get_manifest();
		std::string line = archive == m_archive_repositories.front() ? std::string() : ",";
		line += fmt::format(
			"{{\"name\": \"{}\","
			"\"description\": \"{}\","
			"\"visibility\": \"{}\","
			"\"id\": \"{}\","
			"\"url\": \"{}\","
			"\"index_url\": \"{}\","
			"\"selected\": {},"
            "\"has_installed_printers\": {}"
			"}}"
			, man.name, man.description
			, man. visibility
			, man.id
			, man.url
			, man.index_url
			, is_selected(archive->get_uuid()) ? "1" : "0"
			, has_installed_printers(archive->get_uuid()) ? "1" : "0"
		);
		data += line;
	}
	data += "]";

	std::string path = get_stored_manifest_path().string();
	boost::nowide::ofstream file(path);
	if (file.is_open()) {
		file << data;
		file.close();
	} else {
        throw Slic3r::RuntimeError("Failed to write Archive Repository Manifest to " + path);
	}
}

fs::path PresetUpdaterRepositoryDatabase::get_stored_manifest_path() const
{
	return (boost::filesystem::path(Utils::data_dir()) / "shared_runtime" / "ArchiveRepositoryManifest.json").make_preferred();
}

bool PresetUpdaterRepositoryDatabase::is_selected(const std::string& uuid) const
{
	auto search = m_selected_repositories_uuid.find(uuid);
	assert(search != m_selected_repositories_uuid.end()); 
	return search->second;
}
bool PresetUpdaterRepositoryDatabase::has_installed_printers(const std::string &uuid) const 
{
    auto search = m_has_installed_printer_repositories_uuid.find(uuid);
    assert(search != m_has_installed_printer_repositories_uuid.end());
    return search->second;
}
void PresetUpdaterRepositoryDatabase::clear_online_repos()
{
	auto it = m_archive_repositories.begin();
	while (it != m_archive_repositories.end()) {
		// Do not clean repos with local path (local repo).
        if ((*it)->get_manifest().tmp_path.empty()) {
			it = m_archive_repositories.erase(it);
		} else {
			++it;
		}
	}
}

void PresetUpdaterRepositoryDatabase::read_server_manifest(const std::string& json_body)
{
    nlohmann::json json;
	try
    {
	    json = nlohmann::json::parse(json_body);
    }
    catch (const nlohmann::json::exception& e)
    {
	    SPDLOG_ERROR("Failed to parse JSON. Reason: {}", e.what());
        return;
    }
	// Online repo manifests are in json_body. We already have read local manifest and online manifest from last run.
	// Keep the local ones and replace the online ones but keep uuid for same id so the selected map is correct.
	// Solution: Create id - uuid translate table for online repos.
	std::map<std::string, std::string> id_to_uuid;
	for (const auto& repo_ptr : m_archive_repositories) {
		if (repo_ptr->get_manifest().source_path.empty()){
			id_to_uuid[repo_ptr->get_manifest().id] = repo_ptr->get_uuid();
		}
	}
	
	// Make a stash of secret repos that are online and has installed printers.
	// If some of these will be missing afer reading the json tree, it needs to be added back to main population.
	PrivateArchiveRepositoryVector secret_online_used_repos_cache;
    for (const auto &repo_ptr : m_archive_repositories) {
        if (repo_ptr->get_manifest().visibility.empty() || !repo_ptr->get_manifest().tmp_path.empty()) {
            continue;
		}
        const auto &it = m_has_installed_printer_repositories_uuid.find(repo_ptr->get_uuid());
        assert(it != m_has_installed_printer_repositories_uuid.end());
        if (it->second) {
            PresetUpdaterRepository::RepositoryManifest manifest(repo_ptr->get_manifest());
            secret_online_used_repos_cache.emplace_back(std::make_unique<PresetUpdaterRepositoryOnline>(repo_ptr->get_uuid(), std::move(manifest)));
		}
	}

    clear_online_repos();
	
    DEBUG_ASSERT(json.is_array());
    for (const auto& repo_json : json) {
	    PresetUpdaterRepository::RepositoryManifest manifest;
	    if (!PresetUpdaterRepository::extract_repository_header(repo_json, manifest)) {
		    DEBUG_ASSERT(false);
		    SPDLOG_ERROR("Failed to read one of the repository headers.");
		    continue;
	    }

	    auto id_it = id_to_uuid.find(manifest.id);
	    std::string uuid = (id_it == id_to_uuid.end() ? get_next_uuid() : id_it->second);

	    // Set default selected value to true - its a never before seen repository
	    m_selected_repositories_uuid.try_emplace(uuid, true);
        // Set default "has installed printers" value to false - its a never before seen repository
	    m_has_installed_printer_repositories_uuid.try_emplace(uuid, false);

	    m_archive_repositories.emplace_back(std::make_unique<PresetUpdaterRepositoryOnline>(uuid, std::move(manifest)));
    }
	
	// return missing secret online repos with installed printers to the vector
	for (const auto &repo_ptr : secret_online_used_repos_cache) {
        std::string uuid = repo_ptr->get_uuid();
        if (std::find_if(
                m_archive_repositories.begin(), m_archive_repositories.end(),
                [uuid](const std::unique_ptr<PresetUpdaterRepository> &ptr) {
                    return ptr->get_uuid() == uuid;
                }
            ) == m_archive_repositories.end())
		{
            PresetUpdaterRepository::RepositoryManifest manifest(repo_ptr->get_manifest());
            m_archive_repositories.emplace_back(std::make_unique<PresetUpdaterRepositoryOnline>(repo_ptr->get_uuid(), std::move(manifest)));
	    }
	}

	consolidate_uuid_maps();
    // possible exception catched in PresetUpdaterRepositoryDatabase::sync!
    save_app_manifest_json();
}

SharedArchiveRepositoryVector PresetUpdaterRepositoryDatabase::get_all_archive_repositories() const 
{
    SharedArchiveRepositoryVector result;
    result.reserve(m_archive_repositories.size());
    for (const auto &repo_ptr : m_archive_repositories) 
    {
        result.emplace_back(repo_ptr.get());
    }
    return result;
}

SharedArchiveRepositoryVector PresetUpdaterRepositoryDatabase::get_selected_archive_repositories() const 
{
    SharedArchiveRepositoryVector result;
    result.reserve(m_archive_repositories.size());
    for (const auto &repo_ptr : m_archive_repositories) 
    {
        auto it = m_selected_repositories_uuid.find(repo_ptr->get_uuid());
        assert(it != m_selected_repositories_uuid.end());
        if (it->second) {
            result.emplace_back(repo_ptr.get());
        }   
    }
    return result;
}

bool PresetUpdaterRepositoryDatabase::is_selected_repository_by_uuid(const std::string& uuid) const
{
	auto selected_it = m_selected_repositories_uuid.find(uuid);
	assert(selected_it != m_selected_repositories_uuid.end());
	return selected_it->second;
}
bool PresetUpdaterRepositoryDatabase::is_selected_repository_by_id(const std::string& repo_id) const
{
	assert(!repo_id.empty());
	for (const auto& repo_ptr : m_archive_repositories) {
		if (repo_ptr->get_manifest().id == repo_id) {
			return true;
		}
	}
	return false;
}
void PresetUpdaterRepositoryDatabase::consolidate_uuid_maps()
{
	//std::vector<std::unique_ptr<PresetUpdaterRepository>> m_archive_repositories;
	//std::map<std::string, bool> m_selected_repositories_uuid;
	auto selected_it = m_selected_repositories_uuid.begin();
    while (selected_it != m_selected_repositories_uuid.end()) {
		bool found = false;
		for (const auto& repo_ptr : m_archive_repositories) {
            if (repo_ptr->get_uuid() == selected_it->first) {
				found = true;
				break;	 
			}
		}
		if (!found) {
            selected_it = m_selected_repositories_uuid.erase(selected_it);
		} else {
            ++selected_it;
		}
	}
	// Do the same for m_has_installed_printer_repositories_uuid
    auto installed_it = m_has_installed_printer_repositories_uuid.begin();
    while (installed_it != m_has_installed_printer_repositories_uuid.end()) {
        bool found = false;
        for (const auto &repo_ptr : m_archive_repositories) {
            if (repo_ptr->get_uuid() == installed_it->first) {
                found = true;
                break;
            }
        }
        if (!found) {
            installed_it = m_has_installed_printer_repositories_uuid.erase(installed_it);
        } else {
            ++installed_it;
        }
    }
}

std::string PresetUpdaterRepositoryDatabase::get_next_uuid()
{
	boost::uuids::uuid uuid = m_uuid_generator();
	return boost::uuids::to_string(uuid);
}

namespace {

bool add_authorization_header(Network::IHttp& http, PresetUpdaterProcessStatus* process_status)
{
    const std::string access_token = process_status->access_token();
    if (!access_token.empty()) {
        http.header("Authorization", "Bearer " + access_token);
    }
    return true;
}

bool sync_inner(std::string& manifest, PresetUpdaterProcessStatus* process_status)
{
	bool res = false;
    std::string url = Network::ServiceConfig::instance().preset_repo_repos_url();
    auto retry_fn = [process_status](Network::IHttp::Retry retry, bool& cancel ) {
        cancel = process_status->on_attempt(retry.attempt, retry.ms_to_next_attempt);
    };

    std::unique_ptr<Network::IHttp> http = Network::IHttp::create(Network::IHttp::RequestMethod::Get, url, retry_fn);
    const std::string access_token = process_status->access_token();
    if (!access_token.empty()) {
        http->header("Authorization", "Bearer " + access_token);
    }
    http->timeout_total(30)
        .on_error([&](std::string body, std::string error, unsigned http_status) {
	        SPDLOG_ERROR("Failed to get online archive source manifests: {} ; {} ; {}", body, error, http_status);
            process_status->set_error(error);
            res = false;
		})
		.on_complete([&](std::string body, unsigned /* http_status */) {
			manifest = body;
			res = true;
		})
        .perform_sync(process_status->get_retry_policy());

	return res;
}
}

bool PresetUpdaterRepositoryDatabase::sync(PresetUpdaterProcessStatus* process_status)
{
    assert(process_status);
	std::string manifest;
    bool sync_res = false;
    process_status->set_target("Archive Database Mainfest");
    sync_res = sync_inner(manifest, process_status);
    if (!sync_res) {
        return false;
    }    
    try
    {
        read_server_manifest(std::move(manifest));
    }
    catch (const Slic3r::RuntimeError& e)
    {
        process_status->set_error(e.what());
        return false;
    }
	
    return true;
}

} // namespace Slic3r::Biz::PresetUpdater