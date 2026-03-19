#pragma once

#include <vector>
#include <string>
#include <map>

#include <boost/filesystem/path.hpp>

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterIndex;
class PresetUpdaterProcessStatus;
class PresetUpdaterReconfigurationList;
class AbstractPresetUpdaterRepository;
typedef std::vector<const AbstractPresetUpdaterRepository*> SharedRepositoryVector;

boost::filesystem::path local_presets_path();
boost::filesystem::path local_vendor_path(
    const std::string& repo_id,
    const std::string& vendor_id,
    const std::string& suffix = ""
);

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

PresetUpdaterReconfigurationList check_forced_reconfigurations(
    const SharedRepositoryVector& repos,
    PresetUpdaterProcessStatus* process_status
);
PresetUpdaterReconfigurationList check_reconfigurations(
    const SharedRepositoryVector& repos,
    PresetUpdaterProcessStatus* process_status
);

enum class ReconfigurationType : unsigned int {
    None             = 0,
    ForcedDowngrades = 1 << 0,
    ForcedUpdates    = 1 << 1,
    RegularUpdates   = 1 << 2,
    NotInIndex       = 1 << 3,
    NewVendors       = 1 << 4,
    All              = ForcedDowngrades | ForcedUpdates | RegularUpdates | NotInIndex |NewVendors
};

void perform_reconfigurations(
    const PresetUpdaterReconfigurationList& reconfigurations,
    ReconfigurationType types_to_perform,
    PresetUpdaterProcessStatus* process_status
);

void cleanup_update_sync(PresetUpdaterProcessStatus* process_status);

} // namespace Slic3r::Biz::PresetUpdater
