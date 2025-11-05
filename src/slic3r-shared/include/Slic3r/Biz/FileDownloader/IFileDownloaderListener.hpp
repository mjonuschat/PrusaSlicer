#pragma once

#include <boost/filesystem/path.hpp>

namespace Slic3r::Biz::FileDownloader {

class IFileDownloaderListener
{
public:
    virtual ~IFileDownloaderListener() = default;

    virtual void on_model_downloaded(const boost::filesystem::path& path) = 0;
};

} // namespace Slic3r::App::Scene#pragma once