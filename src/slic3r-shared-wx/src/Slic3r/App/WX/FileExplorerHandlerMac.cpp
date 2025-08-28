#include <Slic3r/App/WX/FileExplorerHandler.hpp>

#include <wx/process.h>
#include <wx/utils.h>
#include <wx/string.h>

namespace Slic3r::App::WX {

void FileExplorerHandler::open_folder(const boost::filesystem::path& path)
{
    const char* argv[] = {"open", path.string().c_str(), nullptr};
    ::wxExecute(argv, wxEXEC_ASYNC, nullptr);
}

} // namespace Slic3r::App::WX
