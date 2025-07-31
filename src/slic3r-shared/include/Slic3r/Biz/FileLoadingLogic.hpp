#pragma once

#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/Model.hpp"

#include <string>
#include <boost/filesystem/path.hpp>

namespace Slic3r::Biz::Scene {
class SceneInteractor;
} // namespace Slic3r::Biz::Scene

namespace Slic3r::Biz::FileLoadingLogic {

/**
 * Loading project from file.
 *
 * @note During loading, the file will be scanned for zero-volume objects.
 * Any found will be automatically removed, and the user will be notified.
 */
Domain::Project load_file_as_project(const boost::filesystem::path& project_file_path);

/**
 * Load meshes (e.g., STL, OBJ) and complex models (e.g., 3MF) from multiple source files
 * and insert them into the scene graph.
 */
void import_files_and_add_to_scene(
    const std::vector<boost::filesystem::path>& input_file_paths,
    int tool_count,
    Scene::SceneInteractor& scene_interactor,
    const Domain::Vec2d& bed_center
);

} // namespace Slic3r::Biz::FileLoadingLogic
