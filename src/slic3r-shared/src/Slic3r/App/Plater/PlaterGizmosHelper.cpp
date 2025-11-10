#include "Slic3r/App/Plater/PlaterGizmosHelper.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/Biz/Scene/Selection.hpp"

namespace Slic3r::App::Plater {

bool can_be_added_to_object_selection(const Scene::Node& node, const Biz::Scene::ObjectSelection& selection)
{
    if (selection.mode == Biz::Scene::SelectionMode::Volume && !selection.empty()) {
        Domain::ElementRef first_sel_vol_ref = selection.elements.front();
        first_sel_vol_ref.volume_id = 0;
        const auto& tag = *node.tag_of_type<SceneNodeTag>();
        Domain::ElementRef vol_ref = { tag.object_id, tag.instance_id, tag.volume_id };
        return vol_ref.is_part_of(first_sel_vol_ref);
    }
    
    return true;
}

} // namespace Slic3r::App::Plater
