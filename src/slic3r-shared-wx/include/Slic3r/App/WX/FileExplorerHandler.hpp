#pragma once

#include "Slic3r/App/Platform/IFileExplorerHandler.hpp"
#include "Slic3r/Biz/Directories.hpp"

namespace Slic3r::App::WX {

class FileExplorerHandler : public Platform::IFileExplorerHandler
{
public:
    void open_folder(const boost::filesystem::path& path) override;

    void open_datadir_folder() override
    {
        boost::filesystem::path path(Biz::data_dir());
        open_folder(path);
    }
};
} // namespace Slic3r::App::WX
