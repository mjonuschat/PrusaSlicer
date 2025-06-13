#pragma once

#include <string>
#include <boost/filesystem/path.hpp>

namespace Slic3r::Biz::PresetUpdater {

struct PresetUpdaterRepositoryDescriptor {
	// mandatory
	std::string id;
	std::string name;
	std::string url;
	// optional
	std::string index_url;
	std::string description;
	std::string visibility;
    std::string offline_archive_url;
	// not read from manifest json
	boost::filesystem::path unzipped_data_path; // Where archive is unzipped. Created each app run. 
	boost::filesystem::path zip_path; // Path given by user. Stored between app runs.

    PresetUpdaterRepositoryDescriptor() = default;
    PresetUpdaterRepositoryDescriptor(
        const std::string &id,
        const std::string &name,
        const std::string &url,
        const std::string &index_url = "",
        const std::string &description = "",
        const std::string &visibility = "",
        const boost::filesystem::path &tmp_path = "",
        const boost::filesystem::path &source_path = ""
    )
        : id(id)
        , name(name)
        , url(url)
        , index_url(index_url)
        , description(description)
        , visibility(visibility)
        , unzipped_data_path(tmp_path)
        , zip_path(source_path) 
	{}
    PresetUpdaterRepositoryDescriptor(const PresetUpdaterRepositoryDescriptor&) = default;
    PresetUpdaterRepositoryDescriptor(PresetUpdaterRepositoryDescriptor&&) noexcept = default;
    PresetUpdaterRepositoryDescriptor& operator=(const PresetUpdaterRepositoryDescriptor&) = default;
    PresetUpdaterRepositoryDescriptor& operator=(PresetUpdaterRepositoryDescriptor&&) noexcept = default;
};

struct SharedPresetUpdaterRepositoryInfo
{
    const PresetUpdaterRepositoryDescriptor descriptor; 
    const std::string uuid;
    bool selected;
};

// This are readonly creadentials of all repositories with setable bool - selected by user.
// This is all UI should need to construct Repository Sources Selection.
typedef std::vector<SharedPresetUpdaterRepositoryInfo> SharedPresetUpdaterRepositoryInfoVector;


}