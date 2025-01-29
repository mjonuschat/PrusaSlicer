#ifndef slic3r_PresetArchiveRepository_hpp_
#define slic3r_PresetArchiveRepository_hpp_

#include <string>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>


namespace PresetManagement {

class PresetUpdaterProcessStatus;

class PresetArchiveRepository
{
public:
	struct RepositoryManifest {
		// mandatory
		std::string id;
		std::string name;
		std::string url;
		// optional
		std::string index_url;
		std::string description;
		std::string visibility;
		// not read from manifest json
		boost::filesystem::path tmp_path; // Where archive is unzipped. Created each app run. 
		boost::filesystem::path source_path; // Path given by user. Stored between app runs.

        RepositoryManifest() = default;
        RepositoryManifest(
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
            , tmp_path(tmp_path)
            , source_path(source_path) 
		{}
        RepositoryManifest(const RepositoryManifest&) = default;
        RepositoryManifest(RepositoryManifest&&) noexcept = default;
        RepositoryManifest& operator=(const RepositoryManifest&) = default;
        RepositoryManifest& operator=(RepositoryManifest&&) noexcept = default;
	};
	// Use std::move when calling constructor.
	PresetArchiveRepository(const std::string& uuid, RepositoryManifest&& data) 
		: m_data(std::move(data))
		, m_uuid(uuid) 
	{}
    PresetArchiveRepository(const PresetArchiveRepository&) = default;
    PresetArchiveRepository(PresetArchiveRepository&&) noexcept = default;
    PresetArchiveRepository& operator=(const PresetArchiveRepository&) = default;
    PresetArchiveRepository& operator=(PresetArchiveRepository&&) noexcept = default;
	virtual ~PresetArchiveRepository()  = default;
	// Gets vendor_indices.zip to target_path
	virtual bool get_archive(const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const = 0;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	virtual bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const = 0;
	// Gets file without id check - for not yet encountered vendors only!
	virtual bool get_ini_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const = 0;
	const RepositoryManifest& get_manifest() const { return m_data; }
	std::string get_uuid() const { return m_uuid; }
    // Only local archives can return false
    virtual bool is_extracted() const { return true; }
    virtual void do_extract() {}
    void set_manifest(RepositoryManifest &&manifest) { m_data = std::move(manifest); }

    static bool extract_repository_header(const boost::property_tree::ptree& ptree, PresetArchiveRepository::RepositoryManifest& data);
protected:
	RepositoryManifest m_data;
	std::string m_uuid;
};

class OnlinePresetArchiveRepository : public PresetArchiveRepository
{
public:
	OnlinePresetArchiveRepository(const std::string& uuid, RepositoryManifest&& data) : PresetArchiveRepository(uuid, std::move(data))
	{
		if (m_data.url.back() != '/') {
			m_data.url += "/";
		}
	}
    OnlinePresetArchiveRepository(const OnlinePresetArchiveRepository&) = default;
    OnlinePresetArchiveRepository(OnlinePresetArchiveRepository&&) noexcept = default;
    OnlinePresetArchiveRepository& operator=(const OnlinePresetArchiveRepository&) = default;
    OnlinePresetArchiveRepository& operator=(OnlinePresetArchiveRepository&&) noexcept = default;
    ~OnlinePresetArchiveRepository() override = default;
	// Gets vendor_indices.zip to target_path.
	bool get_archive(const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const override;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const override;
	// Gets file without checking id.
	// Should be used only if no previous ini file exists.
	bool get_ini_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const override;
private:
	bool get_file_inner(const std::string& url, const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const;
};

class LocalPresetArchiveRepository : public PresetArchiveRepository
{
public:
	LocalPresetArchiveRepository(const std::string& uuid, RepositoryManifest&& data, bool extracted) : PresetArchiveRepository(uuid, std::move(data)), m_extracted(extracted) 
    {}
    LocalPresetArchiveRepository(const LocalPresetArchiveRepository&) = default;
    LocalPresetArchiveRepository(LocalPresetArchiveRepository&&) noexcept = default;
    LocalPresetArchiveRepository& operator=(const LocalPresetArchiveRepository&) = default;
    LocalPresetArchiveRepository& operator=(LocalPresetArchiveRepository&&) noexcept = default;
    ~LocalPresetArchiveRepository() override = default;
	// Gets vendor_indices.zip to target_path.
	bool get_archive(const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const override;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const override;
	// Gets file without checking id.
	// Should be used only if no previous ini file exists.
	bool get_ini_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const override;
    bool is_extracted() const override { return m_extracted;  }
    void do_extract() override;
    
    static bool extract_local_archive_repository(PresetArchiveRepository::RepositoryManifest& manifest_data);
private:
	bool get_file_inner(const boost::filesystem::path& source_path, const boost::filesystem::path& target_path) const;
    bool m_extracted;
};

} // PresetManagement
#endif