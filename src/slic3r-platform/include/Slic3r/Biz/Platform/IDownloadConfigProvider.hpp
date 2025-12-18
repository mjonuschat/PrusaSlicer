#pragma once

#include <boost/filesystem/path.hpp>

namespace Slic3r::Biz::Platform::FileDownloader {

class IDownloadConfigProvider {
public:
  virtual ~IDownloadConfigProvider() = default;
  virtual boost::filesystem::path download_dir() const = 0;
};

} // namespace Slic3r::Biz::FileDownloader