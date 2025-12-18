#pragma once

#include "Slic3r/Biz/Platform/IDownloadConfigProvider.hpp"

namespace Slic3r::App::FileDownloader {

class AppDownloadConfigProvider : public Biz::Platform::FileDownloader::IDownloadConfigProvider {
public:
    boost::filesystem::path download_dir() const override;
};

} // namespace Slic3r::App::FileDownloader