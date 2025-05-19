#include <Slic3r/Biz/PrintHost/PrintHostLocal.hpp>

#include "Slic3r/App/I18N/I18N.hpp"

#include "libslic3r/format.hpp"

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/cstdio.hpp>

namespace fs = boost::filesystem;

namespace Slic3r::Biz::PrintHost {

bool PrintHostLocal::perform(ProgressFn progress_fn, RetryFn retry_fn, ErrorFn error_fn, InfoFn info_fn) const
{
   std::string error;
   bool res =  move_file(m_upload_data.source_path, m_upload_data.dest_path, error);
   if (!res) {
       error_fn(format(_u8L("Failed to export file to %1%. %2%"), m_upload_data.dest_path.string(), error));
   }
   return res; 
}

bool PrintHostLocal::move_file(const boost::filesystem::path& source, const boost::filesystem::path& dest, std::string& msg) const
{
    boost::system::error_code ec;
    ASSERT(fs::exists(source, ec) && !ec);
    ec.clear();
    ASSERT(fs::file_size(source, ec) != 0 && !ec);
    ec.clear();
    ASSERT(fs::exists(dest.parent_path(), ec) && !ec);
    ec.clear();
    
    if (!fs::exists(dest.parent_path(), ec) || ec || !fs::is_directory(dest.parent_path(), ec) || ec) {
        msg = format("Path does not exists: %1%", dest.parent_path().string());
        return false;
    }
    
    ec.clear();
    fs::rename(source, dest, ec); 
    if (ec) {
        msg = format("Failed to move %1% to %2%", source.string(), dest.string());
        return false;
    }
	return true;
}

} // namespace Slic3r::Biz::PrintHost