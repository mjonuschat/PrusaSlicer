#include <Slic3r/Biz/PrintHost/PrintHostLocal.hpp>

#include "Slic3r/App/I18N/I18N.hpp"

#include "libslic3r/format.hpp"

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/cstdio.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

bool PrintHostLocal::perform(PrintHostJobData upload_data, ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
   std::string error;
   bool res =  write_file(upload_data.raw_data, upload_data.dest_path, error);
   if (!res) {
       error_fn(format(_u8L("Failed to export file to %1%. %2%"), upload_data.dest_path.string(), error));
   }
   return res; 
}

bool PrintHostLocal::write_file(const std::string& data, const boost::filesystem::path& path, std::string& msg) const
{
    boost::system::error_code ec;
    if (!fs::exists(path.parent_path(), ec) || ec || !fs::is_directory(path.parent_path(), ec) || ec) {
        msg = _u8L("Path does not exists.");
        return false;
    }
    
    FILE* file; 
    file = boost::nowide::fopen(path.generic_string().c_str(), "wb");
    if (file == NULL) {
         msg = _u8L("Failed to open file for writing.");
        return false;
    }
    fwrite(data.c_str(), 1, data.size(), file);
    fclose(file);

	return true;
}


} // namespace Slic3r::Biz::PrintHost