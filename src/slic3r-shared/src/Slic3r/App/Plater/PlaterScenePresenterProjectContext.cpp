#include "Slic3r/App/Plater/PlaterScenePresenterProjectContext.hpp"
#include "Slic3r/App/Scene/OBBNodeHelper.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"

namespace Slic3r::App::Plater {

static constexpr Scene::RenderLayerId SELECTION_AABB_RENDER_LAYER_ID = Scene::RenderLayerId(0);

void PlaterScenePresenterProjectContext::update_selection_obb_node(Render::Device& device, const Biz::ProjectInteractor& project_interactor)
{
    if (!m_selection_obb_node.dirty)
        return;

    if (!selection_bounding_box.has_value()) {
        if (m_selection_obb_node.top_level_node != nullptr)
            m_selection_obb_node.top_level_node->set_enabled(false);
        if (m_selection_obb_node.volume_nodes_parent != nullptr)
            m_selection_obb_node.volume_nodes_parent->set_enabled(false);
        return;
    }
    else if (m_selection_obb_node.top_level_node != nullptr)
        m_selection_obb_node.top_level_node->set_enabled(true);

    Scene::Scene& scn = scene();
    if (m_selection_obb_node.top_level_node == nullptr) {
        // 
        // create the following hierarchy of nodes:
        // 
        // selection_aabbs - selection_aabb
        //                 - selection_children_aabbs - children_aabb_o_i_v
        //                                            - children_aabb_o_i_v
        //                                            - children_aabb_o_i_v
        //                                            - ...
        // 
        Scene::NodeBuilder builder(scn);
        builder.set_debug_name("selection_aabbs");
        builder.child([&](Scene::NodeBuilder& bldr) {
            Scene::build_obb_node(bldr, model_geometry_manager(), device, "selection_aabb", SELECTION_AABB_RENDER_LAYER_ID);
        });
        builder.child([&](Scene::NodeBuilder& bldr) {
            bldr.set_debug_name("selection_children_aabbs");
            bldr.set_enabled(false);
        });
        auto selection_obbs_node = builder.build().release();
        scn.add_child(selection_obbs_node);
        m_selection_obb_node.top_level_node = selection_obbs_node->children().front().get();
        m_selection_obb_node.volume_nodes_parent = selection_obbs_node->children().back().get();
    }

    DEBUG_ASSERT(m_selection_obb_node.top_level_node != nullptr);
    DEBUG_ASSERT(m_selection_obb_node.volume_nodes_parent != nullptr);
    // updates the selection aabb node
    Scene::update_obb_node(*m_selection_obb_node.top_level_node, *selection_bounding_box, 0.25);

    const Biz::Scene::ObjectSelection& object_selection = project_interactor.scene_interactor().object_selection();
    if (object_selection.mode == Biz::Scene::SelectionMode::Volume) {
        Scene::Node::NodeList nodes;
        // search for required children nodes
        for (const auto& e : object_selection.elements) {
            scn.root().query(
                [&](const Scene::Node* n){
                    const auto* tag = n->tag_of_type<Scene::SceneNodeTag>();
                    return (tag == nullptr) ? false : tag->object_id == e.object_id && tag->instance_id != e.instance_id && tag->volume_id == e.volume_id;
                }, nodes, true );
        }
        // create missing children nodes, if needed
        if (nodes.size() > m_selection_obb_node.volume_nodes_parent->children().size()) {
            for (size_t i = m_selection_obb_node.volume_nodes_parent->children().size(); i < nodes.size(); ++i) {
                Scene::NodeBuilder builder(scn);
                std::string debug_name = fmt::format("children_aabb_{}", i + 1);
                builder.set_debug_name(debug_name);
                Scene::build_obb_node(builder, model_geometry_manager(), device, debug_name,
                    SELECTION_AABB_RENDER_LAYER_ID, Domain::ColorRGB::YELLOW());
                auto children_obb_node = builder.build().release();
                scn.add_child(children_obb_node, m_selection_obb_node.volume_nodes_parent);
            }
        }
        // update children nodes
        for (size_t i = 0; i < nodes.size(); ++i) {
            auto child_node = m_selection_obb_node.volume_nodes_parent->children()[i].get();
            const auto* tag = nodes[i]->tag_of_type<Scene::SceneNodeTag>();
            const Domain::ModelInstance* instance = project_interactor.selected_project().find_instance_by_id(tag->object_id, tag->instance_id);
            const Domain::ModelVolume* volume = project_interactor.selected_project().find_volume_by_id(tag->object_id, tag->volume_id);
            auto world_trafo = instance->get_matrix() * volume->get_matrix();
            Domain::BoundingBox3d aabb = Biz::Algorithms::ModelVolume::transformed_bounding_box(*volume, world_trafo);
            Scene::update_obb_node(*child_node, { 0.5 * (aabb.min + aabb.max), aabb.max - aabb.min }, 0.25);
            child_node->set_debug_name(fmt::format("children_aabb_{}_{}_{}", tag->object_id, tag->instance_id, tag->volume_id));
            child_node->set_enabled(true);
        }
        // disable unused children nodes
        for (size_t i = nodes.size(); i < m_selection_obb_node.volume_nodes_parent->children().size(); ++i) {
            auto child_node = m_selection_obb_node.volume_nodes_parent->children()[i].get();
            child_node->set_debug_name(fmt::format("children_aabb_{}", i + 1));
            child_node->set_enabled(false);
        }

        m_selection_obb_node.volume_nodes_parent->set_enabled(true);
    }
    else
        m_selection_obb_node.volume_nodes_parent->set_enabled(false);

    m_selection_obb_node.dirty = false;
}

} // namespace Slic3r::App::Plater
