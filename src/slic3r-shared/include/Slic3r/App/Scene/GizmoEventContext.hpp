#pragma once

#include <utility>

#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Platform/MouseEvent.hpp"
#include "Slic3r/App/Scene/ISceneChangedListener.hpp"

namespace Slic3r::App::Scene {

class GizmoEventContext : public ISceneChangedListener
{
public:
    GizmoEventContext(
        Scene& scene,
        const Platform::MouseEvent& mouse_event,
        Ray pick_ray,
        NodePickResults pick_results,
        const Render::ScreenInfo& screen_info
    ) :
        m_scene(scene),
        m_mouse_event(mouse_event),
        m_pick_ray(std::move(pick_ray)),
        m_pick_results(std::move(pick_results)),
        m_screen_info(screen_info)
    {
        m_scene.add_listener<ISceneChangedListener>(this);
    }

    GizmoEventContext(const GizmoEventContext& other) :
        m_scene(other.m_scene),
        m_mouse_event(other.m_mouse_event),
        m_pick_ray(other.m_pick_ray),
        m_pick_results(other.m_pick_results),
        m_screen_info(other.m_screen_info)
    {
        m_scene.add_listener<ISceneChangedListener>(this);
    }

    ~GizmoEventContext()
    {
        m_scene.remove_listener<ISceneChangedListener>(this);
    }

    const Platform::MouseEvent& mouse_event() const
    {
        return m_mouse_event;
    }

    const Ray& pick_ray() const
    {
        return m_pick_ray;
    }

    const NodePickResults& pick_results() const
    {
        return m_pick_results;
    }

    const Render::ScreenInfo& screen_info() const
    {
        return m_screen_info;
    }

    float screen_mouse_x() const
    {
        return m_screen_info.mouse_to_screen(m_mouse_event.x());
    }

    float screen_mouse_y() const
    {
        return m_screen_info.mouse_to_screen(m_mouse_event.y());
    }

    NodePickResult* pick_result(const std::function<bool(const NodePickResult&)>& predicate)
    {
        auto it = std::find_if(m_pick_results.begin(), m_pick_results.end(), predicate);
        return it == m_pick_results.end() ? nullptr : &*it;
    }

    const NodePickResult* pick_result(const std::function<bool(const NodePickResult&)>& predicate) const
    {
        auto it = std::find_if(m_pick_results.begin(), m_pick_results.end(), predicate);
        return it == m_pick_results.end() ? nullptr : &*it;
    }

    template <typename T>
    NodePickResult* pick_result_with_tag_of_type()
    {
        return pick_result([](const auto& pr) { return pr.node->template has_tag_of_type<T>(); });
    }

    template <typename T>
    const NodePickResult* pick_result_with_tag_of_type() const
    {
        return pick_result([](const auto& pr) { return pr.node->template has_tag_of_type<T>(); });
    }

    template <typename T>
    Node* pick_result_node_with_tag_of_type()
    {
        auto* r = pick_result_with_tag_of_type<T>();
        return r ? r->node : nullptr;
    }

    template <typename T>
    const Node* pick_result_node_with_tag_of_type() const
    {
        auto* r = pick_result_with_tag_of_type<T>();
        return r ? r->node : nullptr;
    }

    /**
     * @brief Returns true if any of the picked nodes is a GizmoNodeTag or a RotationGizmoNodeTag.
     * @Note This method is temporary until we do not find a better way to handle gizmo picking.
     */
    bool pick_results_contains_gizmo_nodes() const;

    /**
     * @name Implementation of App::Scene::ISceneChangedListener public interface
     * @{
     */
    void on_node_added(App::Scene::Node* node) override {}

    void on_node_removed(App::Scene::Node* node) override
    {
        auto it = std::find_if(
            m_pick_results.begin(),
            m_pick_results.end(),
            [node](const NodePickResult& r) { return r.node == node; }
        );
        if (it != m_pick_results.end())
            m_pick_results.erase(it);
    }

    /**@}*/

private:
    Scene& m_scene;
    Platform::MouseEvent m_mouse_event;
    Ray m_pick_ray;
    NodePickResults m_pick_results;
    Render::ScreenInfo m_screen_info;
};

} // namespace Slic3r::App::Scene
