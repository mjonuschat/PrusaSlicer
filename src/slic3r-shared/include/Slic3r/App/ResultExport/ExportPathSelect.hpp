#pragma once

#include <boost/filesystem/path.hpp>
#include <vector>

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::ExportPathSelect {

/**
 * @brief Shows modal dialog "Save file as". Returns true if user confirmed selection.
 */
void
show_modal_dialog(
    const Biz::ProjectInteractor& project_interactor,
    bool default_path_at_removable,
    const std::function<void(bool result, const std::vector<boost::filesystem::path>& file_paths)>& callback,
    const std::string& wildcards_overide = std::string()
);

} // namespace Slic3r::App::ExportPathSelect
