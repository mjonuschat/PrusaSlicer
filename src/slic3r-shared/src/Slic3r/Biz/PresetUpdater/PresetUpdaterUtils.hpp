#pragma once

#include <vector>
#include <string>
#include <map>

namespace boost::filesystem {
class path;
} // namespace boost::filesystem

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterIndex;
class PresetUpdaterProcessStatus;
class PresetUpdaterReconfigurationList;

bool copy_file_wrapper(
    const boost::filesystem::path& source,
    const boost::filesystem::path& target,
    PresetUpdaterProcessStatus* process_status
);

std::vector<PresetUpdaterIndex> load_vendors_db(const boost::filesystem::path& archive_path);
std::vector<PresetUpdaterIndex> load_vendors_db_filtered(
    const boost::filesystem::path& from_path,
    const std::vector<std::string>& filter
);
std::map<std::string, std::string> read_version_manifest(
    const boost::filesystem::path& path,
    PresetUpdaterProcessStatus* process_status
);

void check_forced_reconfigurations(
    PresetUpdaterReconfigurationList& results,
    PresetUpdaterProcessStatus* process_status
);
void check_reconfigurations(
    PresetUpdaterReconfigurationList& results,
    PresetUpdaterProcessStatus* process_status
);
void perform_reconfigurations(
    const PresetUpdaterReconfigurationList& reconfigurations,
    PresetUpdaterProcessStatus* process_status
);

} // namespace Slic3r::Biz::PresetUpdater
