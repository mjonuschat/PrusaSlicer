#include "Slic3r/App/Plater/SelectionHandler.hpp"
#include "Slic3r/App/Scene/SceneNodeTag.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

#include <unordered_set>

using Slic3r::App::Scene::SceneNodeTag;

namespace Slic3r::App::Plater {

void SelectionHandler::mark_selected(Scene::Node& n, bool replace, bool dragging)
{
    const auto* tag = n.tag_of_type<SceneNodeTag>();
    if (tag == nullptr)
        return;

    Domain::ElementRef element = tag->wipe_tower_id != Domain::SlicingId{} ?
        Domain::ElementRef{tag->wipe_tower_id} :
        Domain::ElementRef{tag->object_id, tag->instance_id, tag->volume_id};

    const Biz::Scene::ObjectSelection& object_selection = m_scene_interactor.object_selection();
    if (object_selection.is_selected(element)) {
        if (element.has_volume() && replace && !dragging) {
            Biz::Scene::ObjectSelection selection{Biz::Scene::SelectionMode::Volume, {element}};
            m_scene_interactor.set_object_selection(selection);
        }
        return;
    }

    Biz::Scene::SelectionMode new_selection_mode{};
    if (element.is_wipe_tower()) {
        new_selection_mode = object_selection.mode;
    } else if (tag->volume_type == Domain::ModelVolumeType::MODEL_PART) {
        new_selection_mode = Biz::Scene::SelectionMode::Instance;
    } else if (replace) {
        new_selection_mode = Biz::Scene::SelectionMode::Volume;
    } else {
        new_selection_mode = object_selection.mode;
    }

    if (new_selection_mode == Biz::Scene::SelectionMode::Instance)
        element.volume_id = 0;

    Biz::Scene::ObjectSelection selection = replace
        ? Biz::Scene::ObjectSelection{new_selection_mode}
        : object_selection;
    if (!selection.remove(element)) {
        selection.elements.push_back(element);
    }
    m_scene_interactor.set_object_selection(selection);
}

void SelectionHandler::mark_unselected(Scene::Node& n)
{
    Biz::Scene::ObjectSelection selection = m_scene_interactor.object_selection();
    const auto* tag = n.tag_of_type<SceneNodeTag>();
    if (tag == nullptr)
        return;

    const Domain::ElementRef element = tag->wipe_tower_id != Domain::SlicingId{} ?
        Domain::ElementRef{tag->wipe_tower_id} :
        Domain::ElementRef{tag->object_id, tag->instance_id, tag->volume_id};
    selection.remove(element);

    m_scene_interactor.set_object_selection(selection);
}

void SelectionHandler::clear_selection()
{
    m_scene_interactor.clear_object_selection();
}

} // namespace Slic3r::App::Plater
