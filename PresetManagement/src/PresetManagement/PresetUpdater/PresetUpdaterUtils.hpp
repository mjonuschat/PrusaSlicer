#ifndef slic3r_PresetUpdaterUtils_hpp_
#define slic3r_PresetUpdaterUtils_hpp_

#include <vector>
#include <string>

namespace boost::filesystem { class path;}

namespace PresetManagement{
class Index;

void copy_file_fix(const boost::filesystem::path &source, const boost::filesystem::path &target);
std::vector<Index> load_vendors_db(const boost::filesystem::path& archive_path);
std::vector<Index> load_vendors_db_filtered(const boost::filesystem::path& from_path, const std::vector<std::string>& filter);
}

#endif