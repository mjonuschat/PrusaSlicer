#include "PresetArchiveRepository.hpp"

#include "PresetUpdaterProcessStatus.hpp"

#include "../../Utils/Format.hpp"
#include "../../Utils/Http.hpp"
#include "../../Utils/miniz_extension.hpp"
#include "../../Utils/Utils.hpp"

#include <boost/log/trivial.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <curl/curl.h>

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

static const char* TMP_EXTENSION = ".download";

namespace PresetManagement {

namespace {
bool unzip_repository(const fs::path& source_path, const fs::path& target_path)
{
	mz_zip_archive archive;
	mz_zip_zero_struct(&archive);
	if (!Slic3r::open_zip_reader(&archive, source_path.string())) {
		BOOST_LOG_TRIVIAL(error) << "Couldn't open zipped Archive source. " << source_path;
		return false;
	}
	size_t num_files = mz_zip_reader_get_num_files(&archive);

	for (size_t i = 0; i < num_files; ++i) {
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
			BOOST_LOG_TRIVIAL(error) << "Failed to get file stat for file #" << i << " in the zip archive. Ending Unzipping.";
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
			BOOST_LOG_TRIVIAL(error) << "Failed to extract file #" << i << " from the zip archive. Ending Unzipping.";
			Slic3r::close_zip_reader(&archive);
			return false;
		}
	}
	Slic3r::close_zip_reader(&archive);
	return true;
}

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
		BOOST_LOG_TRIVIAL(error) << "Failed to delete files at: " << path;
	}
}

std::string escape_string(const std::string& unescaped)
{
	std::string ret_val;
	CURL* curl = curl_easy_init();
	if (curl) {
		char* decoded = curl_easy_escape(curl, unescaped.c_str(), unescaped.size());
		if (decoded) {
			ret_val = std::string(decoded);
			curl_free(decoded);
		}
		curl_easy_cleanup(curl);
	}
	return ret_val;
}
std::string escape_path_by_element(const std::string& path_string)
{
	const boost::filesystem::path path(path_string);
	std::string ret_val = escape_string(path.filename().string());
	boost::filesystem::path parent(path.parent_path());
	while (!parent.empty() && parent.string() != "/") // "/" check is for case "/file.gcode" was inserted. Then boost takes "/" as parent_path.
	{
		ret_val = escape_string(parent.filename().string()) + "/" + ret_val;
		parent = parent.parent_path();
	}
	return ret_val;
}

bool add_authorization_header(Slic3r::Http& http, PresetUpdaterProcessStatus* process_status)
{
    const std::string access_token = process_status->get_access_token();
    if (!access_token.empty()) {
        http.header("Authorization", "Bearer " + access_token);
    }
    return true;
}

} // namespace

bool PresetArchiveRepository::extract_repository_header(const pt::ptree& ptree, PresetArchiveRepository::RepositoryManifest& data)
{
	// mandatory atributes
	if (const auto name = ptree.get_optional<std::string>("name"); name){
		data.name = *name;
	} else {
		BOOST_LOG_TRIVIAL(error) << "Failed to find \"name\" parameter in source manifest. Source is invalid.";
		return false;
	}
	if (const auto id = ptree.get_optional<std::string>("id"); id) {
		data.id = *id;
	}
	else {
		BOOST_LOG_TRIVIAL(error) << "Failed to find \"id\" parameter in source manifest. Source is invalid.";
		return false;
	}
	if (const auto url = ptree.get_optional<std::string>("url"); url) {
		data.url = *url;
	}
	else {
		BOOST_LOG_TRIVIAL(error) << "Failed to find \"url\" parameter in source manifest. Source is invalid.";
		return false;
	}
	// optional atributes
	if (const auto index_url = ptree.get_optional<std::string>("index_url"); index_url) {
		data.index_url = *index_url;
	}
	if (const auto description = ptree.get_optional<std::string>("description"); description) {
		data.description = *description;
	}
	if (const auto visibility = ptree.get_optional<std::string>("visibility"); visibility) {
		data.visibility = *visibility;
	}
	return true;
}

bool OnlinePresetArchiveRepository::get_file_inner(const std::string& url, const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{

	bool res = false;
	fs::path tmp_path = target_path;
	tmp_path += Slic3r::GUI::format(".%1%%2%", /*get_current_pid()*/ "1234", TMP_EXTENSION); // TODO: get_current_pid
	BOOST_LOG_TRIVIAL(info) << Slic3r::GUI::format("Get: `%1%`\n\t-> `%2%`\n\tvia tmp path `%3%`",
		url,
		target_path.string(),
		tmp_path.string());

	auto http = Slic3r::Http::get(url);
    if (!add_authorization_header(http, process_status))
        return false;
    http
		.timeout_max(30)
		.on_progress([](Slic3r::Http::Progress, bool& cancel) {
			//if (cancel) { cancel = true; }
		})
		.on_error([&](std::string body, std::string error, unsigned http_status) {
			BOOST_LOG_TRIVIAL(error) << Slic3r::GUI::format("Error getting: `%1%`: HTTP %2%, %3%", url, http_status, body);
             process_status->set_error(error);
             res = false;
		})
		.on_complete([&](std::string body, unsigned /* http_status */) {
			if (body.empty()) {
				return;
			}
			fs::fstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
			file.write(body.c_str(), body.size());
			file.close();
			fs::rename(tmp_path, target_path);
			res = true;
		})
        .on_retry([&](int attempt, unsigned delay) {
            return !process_status->on_attempt(attempt, delay);
		})
		.perform_sync(process_status->get_retry_policy());	

	return res;
}

bool OnlinePresetArchiveRepository::get_archive(const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{
	return get_file_inner(m_data.index_url.empty() ? m_data.url + "vendor_indices.zip" : m_data.index_url, target_path, process_status);
}

bool OnlinePresetArchiveRepository::get_file(const std::string& source_subpath, const fs::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const
{
	if (repository_id != m_data.id) {
		BOOST_LOG_TRIVIAL(error) << "Error getting file " << source_subpath << ". The repository_id was not matching.";
	    return false;
	}
    
    process_status->set_target(target_path.filename().string());
    
	const std::string escaped_source_subpath = escape_path_by_element(source_subpath);
	return get_file_inner(m_data.url + escaped_source_subpath, target_path, process_status);
}

bool OnlinePresetArchiveRepository::get_ini_no_id(const std::string& source_subpath, const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{
    process_status->set_target(target_path.filename().string());
    
	const std::string escaped_source_subpath = escape_path_by_element(source_subpath);
	return get_file_inner(m_data.url + escaped_source_subpath, target_path, process_status);
}

bool LocalPresetArchiveRepository::get_file_inner(const fs::path& source_path, const fs::path& target_path) const
{
	BOOST_LOG_TRIVIAL(debug) << Slic3r::GUI::format("Copying %1% to %2%", source_path, target_path);
	std::string error_message;
	Slic3r::CopyFileResult cfr = Slic3r::copy_file(source_path.string(), target_path.string(), error_message, false);
	if (cfr != Slic3r::CopyFileResult::SUCCESS) {
		BOOST_LOG_TRIVIAL(error) << "Copying of " << source_path << " to " << target_path << " has failed (" << cfr << "): " << error_message;
		// remove target file, even if it was there before
        boost::system::error_code ec;
		if (fs::exists(target_path, ec) && !ec) {
            ec.clear();
			fs::remove(target_path, ec);
			if (ec) {
				BOOST_LOG_TRIVIAL(error) << Slic3r::GUI::format("Failed to delete file: %1%", ec.message());
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

bool LocalPresetArchiveRepository::get_file(const std::string& source_subpath, const fs::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const
{
	if (repository_id != m_data.id) {
		BOOST_LOG_TRIVIAL(error) << "Error getting file " << source_subpath << ". The repository_id was not matching.";
		return false;
	}
	return get_file_inner(m_data.tmp_path / source_subpath, target_path);
}
bool LocalPresetArchiveRepository::get_ini_no_id(const std::string& source_subpath, const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{
	return get_file_inner(m_data.tmp_path / source_subpath, target_path);
}
bool LocalPresetArchiveRepository::get_archive(const fs::path& target_path, PresetUpdaterProcessStatus* process_status) const
{
	fs::path source_path = fs::path(m_data.tmp_path) / "vendor_indices.zip";
	return get_file_inner(std::move(source_path), target_path);
}

void LocalPresetArchiveRepository::do_extract() 
{
    RepositoryManifest new_manifest;
    new_manifest.source_path = this->get_manifest().source_path;
    new_manifest.tmp_path = this->get_manifest().tmp_path;
    m_extracted = extract_local_archive_repository(new_manifest);
    set_manifest(std::move(new_manifest));
}

bool LocalPresetArchiveRepository::extract_local_archive_repository(PresetArchiveRepository::RepositoryManifest& manifest_data)
{
    assert(!manifest_data.tmp_path.empty());
    assert(!manifest_data.source_path.empty());
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
		pt::ptree ptree;
		pt::read_json(manifest_path.string(), ptree);
		if (!extract_repository_header(ptree, manifest_data)) {
            BOOST_LOG_TRIVIAL(error) << "Failed to load source " << manifest_data.tmp_path;
			return false;
		}
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to read source manifest JSON " << manifest_path << ". reason: " << e.what();
		return false;
	}
	return true;
}

} // PresetManagement