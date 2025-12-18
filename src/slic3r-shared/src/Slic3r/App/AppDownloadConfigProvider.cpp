#include "Slic3r/App/AppDownloadConfigProvider.hpp"
#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/AppConfig.hpp"
#include "Slic3r/Directories.hpp"
#include "Slic3r/Log.hpp"

#include <boost/filesystem.hpp>

namespace Slic3r::App::FileDownloader {


boost::filesystem::path AppDownloadConfigProvider::download_dir() const
{
    boost::filesystem::path dest_dir{AppServices::instance().app_config().get<std::string>("downloads_directory")};
    // The app config value is currently not checked after user change in preferences dialog.
    // If not exist and cannot be created - use default.
    boost::system::error_code ec;
    if (boost::filesystem::exists(dest_dir, ec) && !ec && boost::filesystem::is_directory(dest_dir, ec) && !ec)
    {
        return dest_dir;
    }
    if (!boost::filesystem::create_directories(dest_dir, ec) || ec) 
    {
        boost::filesystem::path system_dir = system_downloads_dir();
        SPDLOG_ERROR("Failed to create directory for downloads ({}). Using default path instead: {}", dest_dir.string(), system_dir.string());
        return system_dir;
    }
    return dest_dir;
}


} // Slic3r::App::FileDownloader