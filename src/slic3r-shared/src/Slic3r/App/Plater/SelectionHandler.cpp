#include "Slic3r/App/Plater/SelectionHandler.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"

#include <unordered_set>

namespace Slic3r::App::Plater {

void SelectionHandler::mark_selected(Scene::Node& n, bool replace)
{
    const auto* tag = n.tag_of_type<SceneNodeTag>();
    if (tag == nullptr)
        return;

    Domain::ElementRef element = {tag->object_id, tag->instance_id, tag->volume_id};

    if (m_scene_interactor.selection().is_selected(element))
        return;

    auto selection_mode = m_scene_interactor.selection().mode;

    auto new_selection_mode = tag->volume_type == ModelVolumeType::MODEL_PART ?
        Biz::Scene::SelectionMode::Instance :
        replace ? Biz::Scene::SelectionMode::Volume : selection_mode;

    if (new_selection_mode == Biz::Scene::SelectionMode::Instance)
        element.volume_id = 0;
    Biz::Scene::Selection selection = replace
        ? Biz::Scene::Selection{new_selection_mode}
        : m_scene_interactor.selection();
    if (!selection.remove(element)) {
        selection.elements.push_back(element);
        selection.normalize();
    }
    m_scene_interactor.set_selection(selection);
}

void SelectionHandler::mark_unselected(Scene::Node& n)
{
    Biz::Scene::Selection selection = m_scene_interactor.selection();
    const auto* tag = n.tag_of_type<SceneNodeTag>();
    if (tag == nullptr)
        return;
    selection.elements.erase(
        std::remove_if(
            selection.elements.begin(), selection.elements.end(),
            [tag](const auto& e) { return tag->matches_element(e); }
        ),
        selection.elements.end()
    );

    m_scene_interactor.set_selection(selection);
}

void SelectionHandler::clear_selection()
{
    m_scene_interactor.set_selection({Biz::Scene::SelectionMode::Volume});
}

}
