#include "Slic3r/Biz/VolumeReloadLogic.hpp"

#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"
#include "Slic3r/Biz/FileLoadingLogic.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/IMessageDialogProvider.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/Biz/Scene/SelectionExtents.hpp"
#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/ModelVolume.hpp"
#include "Slic3r/Domain/Project.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <optional>
#include <string>
#include <utility>

using Slic3r::Biz::Scene::SceneInteractor;
using Slic3r::Biz::Scene::SelectionExtents;
using Slic3r::Domain::ElementRef;
using Slic3r::Domain::ElementRefs;
using Slic3r::Domain::Model;
using Slic3r::Domain::ModelObject;
using Slic3r::Domain::ModelVolume;
using Slic3r::Domain::Project;
using Slic3r::Domain::SquareMatrix4d;

namespace Slic3r::Biz::VolumeReloadLogic {

namespace {

tl::expected<void, std::string> replace_volume_from_file(
    const ElementRef& volume_element,
    const boost::filesystem::path& new_file_path,
    const Project& project,
    SceneInteractor& scene_interactor,
    IMessageDialogProvider* dialog_provider
)
{
    tl::expected<Model, std::string> loaded_model =
        FileLoadingLogic::read_model_from_file(new_file_path.string(), dialog_provider);
    if (!loaded_model) {
        return tl::make_unexpected(loaded_model.error());
    }

    if (loaded_model->objects.size() != 1 || loaded_model->objects.front()->volumes.size() != 1) {
        return tl::make_unexpected(_u8L("Unable to replace with more than one volume"));
    }

    const ModelVolume* old_volume =
        project.find_volume_by_id(volume_element.object_id, volume_element.volume_id);
    if (old_volume == nullptr) {
        return tl::make_unexpected(_u8L("Volume to replace was not found"));
    }

    const ModelVolume& new_volume  = *loaded_model->objects.front()->volumes.front();
    ModelVolume::Source new_source = new_volume.source;
    new_source.input_file          = new_file_path.string();
    new_source.object_idx          = 0;
    new_source.volume_idx          = 0;

    SceneInteractor::VolumeMeshReplacement replacement = {
        .element            = volume_element,
        .mesh               = new_volume.mesh(),
        .new_source         = std::move(new_source),
        .new_transformation = old_volume->get_matrix()
            * Domain::translation_transform(
                                  new_volume.source.mesh_offset - old_volume->source.mesh_offset
            ),
        .new_name = new_volume.name.empty() ? new_file_path.filename().string() : new_volume.name
    };

    SceneInteractor::VolumeMeshReplacements replacements;
    replacements.push_back(std::move(replacement));
    scene_interactor.change_volume_meshes(std::move(replacements));

    const ModelObject* object = project.find_object_by_id(volume_element.object_id);
    if (object != nullptr && object->volumes.size() == 1) {
        const std::optional<SelectionExtents> selection_extents =
            scene_interactor.selection_bounding_box();
        if (selection_extents.has_value() && selection_extents->is_floating()) {
            SquareMatrix4d drop_transform = SquareMatrix4d::Identity();
            drop_transform.col(3).z()     = -selection_extents->min_z();
            scene_interactor.transform_selection(drop_transform);
        }
    }

    return {};
}

} // namespace

std::string proposed_replace_file_name(const SceneInteractor& scene_interactor)
{
    const ElementRefs volume_refs = scene_interactor.selected_volumes();
    if (volume_refs.size() != 1) {
        return {};
    }

    const ElementRef& volume_element = volume_refs.front();
    const ModelVolume* volume        = scene_interactor.selected_project().find_volume_by_id(
        volume_element.object_id,
        volume_element.volume_id
    );
    if (volume == nullptr
        || volume->source.input_file.empty()
        || !boost::filesystem::exists(volume->source.input_file))
    {
        return {};
    }

    return boost::filesystem::path(volume->source.input_file).filename().string();
}

tl::expected<void, std::string> replace_selected_volume(
    const boost::filesystem::path& new_file_path,
    SceneInteractor& scene_interactor,
    IMessageDialogProvider* dialog_provider
)
{
    const ElementRefs volume_refs = scene_interactor.selected_volumes();
    if (volume_refs.size() != 1) {
        return tl::make_unexpected(_u8L("Volume to replace was not found"));
    }

    return replace_volume_from_file(
        volume_refs.front(),
        new_file_path,
        scene_interactor.selected_project(),
        scene_interactor,
        dialog_provider
    );
}

} // namespace Slic3r::Biz::VolumeReloadLogic
