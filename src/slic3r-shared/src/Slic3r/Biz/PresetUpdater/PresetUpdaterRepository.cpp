#include "Slic3r/Biz/PresetUpdater/PresetUpdaterRepository.hpp"

#include "Slic3r/Biz/PresetUpdater/PresetUpdaterProcessStatus.hpp"
#include "Slic3r/Biz/Network/IHttp.hpp"
#include "Slic3r/Biz/CopyFile.hpp"

#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

#include "libslic3r/miniz_extension.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <fmt/format.h>

namespace fs = boost::filesystem;
static const char* TMP_EXTENSION = ".download";

namespace Slic3r::Biz::PresetUpdater {

namespace {

void delete_path_recursive(const fs::path& path)
{
	try {
        boost::system::error_code ec;
		if (fs::exists(path, ec) && !ec) {
			for (fs::directory_iterator it(path); it != fs::directory_iterator(); ++it) {
				const fs::path subpath = it->path();
				if (fs::is_directory(subpath)) {
					delete_path_recursive(subpath);
				} else {
					fs::remove(subpath);
				}
			}
			fs::remove(path);
		}
	}
	catch (const std::exception&) {
		SPDLOG_ERROR("Failed to delete files at: {}", path.string());
	}
}

bool unzip_repository(const fs::path& source_path, const fs::path& target_path)
{
	mz_zip_archive archive;
	mz_zip_zero_struct(&archive);
	if (!Slic3r::open_zip_reader(&archive, source_path.string())) {
		SPDLOG_ERROR("Couldn't open zipped Archive source. {}", source_path.string());
		return false;
	}
	size_t num_files = mz_zip_reader_get_num_files(&archive);

	for (size_t i = 0; i < num_files; ++i) {
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
			SPDLOG_ERROR("Failed to get file stat for file #{} in the zip archive. Ending Unzipping.", std::to_string(i));
			Slic3r::close_zip_reader(&archive);
			return false;
		}
		fs::path extracted_path = target_path / file_stat.m_filename;
		if (file_stat.m_is_directory) {
			// Create directory if it doesn't exist
			fs::create_directories(extracted_path);
			continue;
		}
		// Create parent directory if it doesn't exist
		fs::create_directories(extracted_path.parent_path());
		// Extract file
		if (!mz_zip_reader_extract_to_file(&archive, i, extracted_path.string().c_str(), 0)) {
			SPDLOG_ERROR("Failed to extract file #{} from the zip archive. Ending Unzipping.", i);
			Slic3r::close_zip_reader(&archive);
			return false;
		}
	}
	Slic3r::close_zip_reader(&archive);
	return true;
}

} // namespace

bool PresetUpdaterRepository::extract_repository_header(const nlohmann::json& json, PresetUpdaterRepository::RepositoryManifest& data)
{
    try {
		// Mandatory attributes
		json.at("name").get_to(data.name);
		json.at("id").get_to(data.id);
		json.at("url").get_to(data.url);

		// Optional attributes
		if (json.contains("index_url")) {
			json.at("index_url").get_to(data.index_url);
		}
		if (json.contains("description")) {
			json.at("description").get_to(data.description);
		}
		if (json.contains("visibility")) {
			json.at("visibility").get_to(data.visibility);
		}
	} catch (const nlohmann::json::exception& e) {
		// Catches missing mandatory keys and any type errors
		SPDLOG_ERROR("Failed to parse source manifest: " + std::string(e.what()));
		return false;
	}
	return true;
}

bool PresetUpdaterRepositoryOnline::get_file_inner(const std::string& url, const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{
    boost::uuids::random_generator generator;
    boost::uuids::uuid uuid = generator();

	bool res = false;
	fs::path tmp_path = target_path;
	tmp_path += fmt::format(".{}{}", boost::uuids::to_string(generator()), TMP_EXTENSION); 

	SPDLOG_INFO("Get: `{}`\n\t-> `{}`\n\tvia tmp path `{}`",
		url,
		target_path.string(),
		tmp_path.string());


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
			SPDLOG_ERROR("Error getting: `{}`: HTTP {}, {}", url, http_status, body);
             process_status->set_error(error);
             res = false;
		})
		.on_complete([&](std::string body, unsigned /* http_status */) {
			if (body.empty()) {
				return;
			}
			boost::nowide::fstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
			file.write(body.c_str(), body.size());
			file.close();
			fs::rename(tmp_path, target_path);
			res = true;
		})
        .perform_sync(process_status->get_retry_policy());	

	return res;
}

bool PresetUpdaterRepositoryOnline::get_archive(const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{
	return get_file_inner(m_data.index_url.empty() ? m_data.url + "vendor_indices.zip" : m_data.index_url, target_path, process_status);
}

bool PresetUpdaterRepositoryOnline::get_file(const std::string& source_subpath, const fs::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const
{
	if (repository_id != m_data.id) {
		SPDLOG_ERROR("Error getting file {}. The repository_id was not matching.", source_subpath);
	    return false;
	}
    
    process_status->set_target(target_path.filename().string());
    
	const std::string escaped_source_subpath = Network::IHttp::escape_path_by_element(fs::path(source_subpath));
	return get_file_inner(m_data.url + escaped_source_subpath, target_path, process_status);
}

bool PresetUpdaterRepositoryOnline::get_ini_no_id(const std::string& source_subpath, const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{
    process_status->set_target(target_path.filename().string());
    
	const std::string escaped_source_subpath = Network::IHttp::escape_path_by_element(fs::path(source_subpath));
	return get_file_inner(m_data.url + escaped_source_subpath, target_path, process_status);
}

bool PresetUpdaterRepositoryLocal::get_file_inner(const fs::path& source_path, const fs::path& target_path) const
{
	SPDLOG_INFO("Copying {} to {}", source_path.string(), target_path.string());
	std::string error_message;
	Utils::CopyFileResult cfr = Utils::copy_file(source_path.string(), target_path.string(), error_message, false);
	if (cfr != Utils::CopyFileResult::Success) {
        SPDLOG_ERROR("Copying of {} to {} has failed: {}", source_path.string(), target_path.string(), error_message);
		// remove target file, even if it was there before
        boost::system::error_code ec;
		if (fs::exists(target_path, ec) && !ec) {
            ec.clear();
			fs::remove(target_path, ec);
			if (ec) {
				SPDLOG_ERROR("Failed to delete file: {}", ec.message());
			}
		}
		return false;
	}
	// Permissions should be copied from the source file by copy_file(). We are not sure about the source
	// permissions, let's rewrite them with 644.
	static constexpr const auto perms = fs::owner_read | fs::owner_write | fs::group_read | fs::others_read;
	fs::permissions(target_path, perms);

	return true;
}

bool PresetUpdaterRepositoryLocal::get_file(const std::string& source_subpath, const fs::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const
{
	if (repository_id != m_data.id) {
		SPDLOG_ERROR("Error getting file {}. The repository_id was not matching.", source_subpath);
		return false;
	}
	return get_file_inner(m_data.tmp_path / source_subpath, target_path);
}
bool PresetUpdaterRepositoryLocal::get_ini_no_id(const std::string& source_subpath, const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{
	return get_file_inner(m_data.tmp_path / source_subpath, target_path);
}
bool PresetUpdaterRepositoryLocal::get_archive(const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{
	fs::path source_path = fs::path(m_data.tmp_path) / "vendor_indices.zip";
	return get_file_inner(std::move(source_path), target_path);
}

void PresetUpdaterRepositoryLocal::do_extract() 
{
    RepositoryManifest new_manifest;
    new_manifest.source_path = this->get_manifest().source_path;
    new_manifest.tmp_path = this->get_manifest().tmp_path;
    m_extracted = extract_local_archive_repository(new_manifest);
    set_manifest(std::move(new_manifest));
}

bool PresetUpdaterRepositoryLocal::extract_local_archive_repository(PresetUpdaterRepository::RepositoryManifest& manifest_data)
{
    DEBUG_ASSERT(!manifest_data.tmp_path.empty());
    DEBUG_ASSERT(!manifest_data.source_path.empty());
	// Delete previous data before unzip.
	// We have unique path in temp set for whole run of slicer and in it folder for each repo. 
	delete_path_recursive(manifest_data.tmp_path);
	fs::create_directories(manifest_data.tmp_path);
	// Unzip repository zip to unique path in temp directory.
    if (!unzip_repository(manifest_data.source_path, manifest_data.tmp_path)) {
		return false;
	}
	// Read the manifest file.
	fs::path manifest_path = manifest_data.tmp_path / "manifest.json";


    try
    {
	    boost::nowide::ifstream file_stream(manifest_path.c_str());
	    if (!file_stream.is_open()) {
		    SPDLOG_ERROR("Failed to open manifest file: {}", manifest_path.string());
		    return false;
	    }

	    nlohmann::json j = nlohmann::json::parse(file_stream, nullptr, false);
	    if (j.is_discarded()) {
		    SPDLOG_ERROR("Failed to parse manifest as JSON: {}", manifest_path.string());
		    return false;
	    }
	
	    if (!extract_repository_header(j, manifest_data)) {
		    SPDLOG_ERROR("Failed to process repository data for source: {}", manifest_data.id);
		    return false;
	    }
    }
    catch (const std::exception& e)
    {
	    SPDLOG_ERROR("Failed to read source manifest JSON {}. Reason: {}", manifest_path.string(), e.what());
	    return false;
    }
	return true;
}


} // namespace Slic3r::Biz::PresetUpdater