#pragma once

#include <string>

#include <boost/filesystem/path.hpp>
#include <tl/expected.hpp>

namespace Slic3r::Biz {
class IMessageDialogProvider;

namespace Scene {
class SceneInteractor;
} // namespace Scene
} // namespace Slic3r::Biz

namespace Slic3r::Biz::VolumeReloadLogic {

/**
 * @brief File name to prefill the "Select the new file" dialog with.
 *
 * The source file of the selected volume when it still exists on disk, empty otherwise.
 */
std::string proposed_replace_file_name(const Scene::SceneInteractor& scene_interactor);

/**
 * @brief Replaces the mesh of the selected volume with the content of the given file.
 */
tl::expected<void, std::string> replace_selected_volume(
    const boost::filesystem::path& new_file_path,
    Scene::SceneInteractor& scene_interactor,
    IMessageDialogProvider* dialog_provider
);

} // namespace Slic3r::Biz::VolumeReloadLogic
