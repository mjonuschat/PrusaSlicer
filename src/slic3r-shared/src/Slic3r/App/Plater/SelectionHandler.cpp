#include "Slic3r/App/Plater/SelectionHandler.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"

namespace Slic3r::App::Plater {

void SelectionHandler::mark_selected(Scene::Node& n, bool replace)
{
    Biz::Scene::Selection selection = replace ? Biz::Scene::Selection{} : m_scene_interactor.selection();

    auto& selection_changes = m_scene_provider.selection_scene_changes();
    if (replace)
        selection_changes.roll_back();
    selection_changes.change(n).set_material_override(
        Scene::Material{}.set_uniform("uniform_color", ColorRGBA{1.0f, 1.0f, 1.0f, 1.0f})
    );
    const auto* tag = n.tag_of_type<SceneNodeTag>();
    if (tag)
    {
        selection.elements.push_back({tag->object_id, tag->instance_id, tag->volume_id});
    }

    m_scene_interactor.set_selection(selection);
}

void SelectionHandler::mark_unselected(Scene::Node& n)
{
    auto& selection_changes = m_scene_provider.selection_scene_changes();
    selection_changes.roll_back_node(&n);
}

void SelectionHandler::clear_selection()
{
    auto& selection_changes = m_scene_provider.selection_scene_changes();
    selection_changes.roll_back();
    m_scene_interactor.set_selection({});
}


}
