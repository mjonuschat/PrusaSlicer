#pragma once

#include "Slic3r/Biz/PresetUpdater/PresetUpdaterRepositoryCredentials.hpp"

#include <string>
#include <boost/filesystem/path.hpp>

#include "nlohmann/json.hpp"

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterProcessStatus;

class AbstractPresetUpdaterRepository
{
public:
	// Use std::move when calling constructor.
	AbstractPresetUpdaterRepository(const std::string& uuid, PresetUpdaterRepositoryDescriptor&& data, bool selected) 
		: m_data(std::move(data))
		, m_uuid(uuid) 
        , m_selected(selected)
	{}
    AbstractPresetUpdaterRepository(const AbstractPresetUpdaterRepository&) = default;
    AbstractPresetUpdaterRepository(AbstractPresetUpdaterRepository&&) noexcept = default;
    AbstractPresetUpdaterRepository& operator=(const AbstractPresetUpdaterRepository&) = default;
    AbstractPresetUpdaterRepository& operator=(AbstractPresetUpdaterRepository&&) noexcept = default;
	virtual ~AbstractPresetUpdaterRepository()  = default;
	// Gets vendor_indices.zip to target_path
	virtual bool get_archive(const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const = 0;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	virtual bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const = 0;
	virtual bool get_file_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const = 0;
	const PresetUpdaterRepositoryDescriptor& get_descriptor() const { return m_data; }
	std::string get_uuid() const { return m_uuid; }
    void set_descriptor(PresetUpdaterRepositoryDescriptor&& descriptor) { m_data = std::move(descriptor); }
    bool is_selected() { return m_selected; }
    void set_selected(bool selected) { m_selected = selected; }

    static bool extract_repository_header(const nlohmann::json& json, PresetUpdaterRepositoryDescriptor& data, PresetUpdaterProcessStatus* process_status);

protected:
	PresetUpdaterRepositoryDescriptor m_data;
	std::string m_uuid;
    bool m_selected;
};

class OnlinePresetUpdaterRepository : public AbstractPresetUpdaterRepository
{
public:
	OnlinePresetUpdaterRepository(const std::string& uuid, PresetUpdaterRepositoryDescriptor&& data, bool selected) : AbstractPresetUpdaterRepository(uuid, std::move(data), selected)
	{
		if (m_data.url.back() != '/') {
			m_data.url += "/";
		}
	}
    OnlinePresetUpdaterRepository(const OnlinePresetUpdaterRepository&) = default;
    OnlinePresetUpdaterRepository(OnlinePresetUpdaterRepository&&) noexcept = default;
    OnlinePresetUpdaterRepository& operator=(const OnlinePresetUpdaterRepository&) = default;
    OnlinePresetUpdaterRepository& operator=(OnlinePresetUpdaterRepository&&) noexcept = default;
    ~OnlinePresetUpdaterRepository() override = default;
	// Gets vendor_indices.zip to target_path.
	bool get_archive(const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const override;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const override;
	// Gets file without checking id.
     bool get_file_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const override;
private:
	bool get_file_inner(const std::string& url, const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const;
};

class LocalPresetUpdaterRepository : public AbstractPresetUpdaterRepository
{
public:
	LocalPresetUpdaterRepository(const std::string& uuid, PresetUpdaterRepositoryDescriptor&& data, bool selected  ) : AbstractPresetUpdaterRepository(uuid, std::move(data), selected)
    {}
    LocalPresetUpdaterRepository(const LocalPresetUpdaterRepository&) = default;
    LocalPresetUpdaterRepository(LocalPresetUpdaterRepository&&) noexcept = default;
    LocalPresetUpdaterRepository& operator=(const LocalPresetUpdaterRepository&) = default;
    LocalPresetUpdaterRepository& operator=(LocalPresetUpdaterRepository&&) noexcept = default;
    ~LocalPresetUpdaterRepository() override = default;
	// Gets vendor_indices.zip to target_path.
	bool get_archive(const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const override;
	// Gets file if repository_id arg matches m_id.
	// Should be used to get the most recent ini file and every missing resource. 
	bool get_file(const std::string& source_subpath, const boost::filesystem::path& target_path, const std::string& repository_id, PresetUpdaterProcessStatus* process_status) const override;
	// Gets file without checking id.
	// Should be used only if no previous ini file exists.
	//bool get_ini_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const override;
    bool get_file_no_id(const std::string& source_subpath, const boost::filesystem::path& target_path, PresetUpdaterProcessStatus* process_status) const override;
    
    static bool extract_local_archive_repository(PresetUpdaterRepositoryDescriptor& manifest_data, PresetUpdaterProcessStatus* process_status);
private:
	bool get_file_inner(const boost::filesystem::path& source_path, const boost::filesystem::path& target_path) const;
};

} // namespace Slic3r::Biz::PresetUpdater