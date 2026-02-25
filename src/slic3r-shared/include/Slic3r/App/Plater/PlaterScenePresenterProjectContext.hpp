#pragma once

#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"
#include "Slic3r/App/Plater/SinkingContours.hpp"

namespace Slic3r::App::Plater {

class PlaterScenePresenterProjectContext : public Scene::ScenePresenterProjectContext
{
public:
    PlaterScenePresenterProjectContext() = default;
    PlaterScenePresenterProjectContext(const PlaterScenePresenterProjectContext&) = delete;
    PlaterScenePresenterProjectContext& operator=(const PlaterScenePresenterProjectContext&) = delete;
    PlaterScenePresenterProjectContext(PlaterScenePresenterProjectContext&&) = default;

    SinkingContours& sinking_contours() { return m_sinking_contours; }
    const SinkingContours& sinking_contours() const { return m_sinking_contours; }
    void set_sinking_contours_highlight_enabled(bool enable) { m_sinking_contours.set_highlight_enabled(enable); }

    void set_selection_obb_node_as_dirty() { m_selection_obb_node.dirty = true; }
    void update_selection_obb_node(Render::Device& device, const Biz::ProjectInteractor& project_interactor);
    void set_selection_obb_visible(bool visible);

private:
    SinkingContours m_sinking_contours;
    struct SelectionOBBNode
    {
        Scene::Node* main_node{ nullptr };
        Scene::Node* selection_node{ nullptr };
        bool dirty{ true };
        bool visible{ true };
        Scene::Node* volume_nodes_parent{ nullptr };
    };
    SelectionOBBNode m_selection_obb_node;
};

} // namespace Slic3r::App::Plater
