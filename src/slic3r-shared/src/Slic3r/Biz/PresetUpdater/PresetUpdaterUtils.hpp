#pragma once



#include <vector>
#include <string>

namespace boost::filesystem { class path;}

namespace Slic3r::Biz::PresetUpdater {

class PresetUpdaterIndex;
class PresetUpdaterProcessStatus;
class PresetUpdaterReconfigurationList;

void copy_file_fix(const boost::filesystem::path &source, const boost::filesystem::path &target);
std::vector<PresetUpdaterIndex> load_vendors_db(const boost::filesystem::path& archive_path);
std::vector<PresetUpdaterIndex> load_vendors_db_filtered(const boost::filesystem::path& from_path, const std::vector<std::string>& filter);


void check_forced_reconfigurations(PresetUpdaterReconfigurationList& results, PresetUpdaterProcessStatus* process_status);
void check_reconfigurations(PresetUpdaterReconfigurationList& results, PresetUpdaterProcessStatus* process_status);
void perform_reconfigurations(const PresetUpdaterReconfigurationList& reconfigurations, PresetUpdaterProcessStatus* process_status);

} // namespace Slic3r::Biz::PresetUpdater