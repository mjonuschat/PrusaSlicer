#pragma once

#include <utility>

#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Platform/MouseEvent.hpp"

namespace Slic3r::App::Plater {

class GizmoEventContext {
public:
    GizmoEventContext(
        const Platform::MouseEvent& mouse_event,
        Scene::Ray  pick_ray,
        Scene::NodePickResults  pick_results,
        const Render::ScreenInfo& screen_info
    )
        : m_mouse_event(mouse_event)
        , m_pick_ray(std::move(pick_ray))
        , m_pick_results(std::move(pick_results))
        , m_screen_info(screen_info)
    {}

    const Platform::MouseEvent& mouse_event() const { return m_mouse_event; }
    const Scene::Ray& pick_ray() const { return m_pick_ray; }
    const Scene::NodePickResults& pick_results() const { return m_pick_results; }
    const Render::ScreenInfo& screen_info() const { return m_screen_info; }

    float screen_mouse_x() const { return m_screen_info.mouse_to_screen(m_mouse_event.x()); }
    float screen_mouse_y() const { return m_screen_info.mouse_to_screen(m_mouse_event.y()); }

    const Scene::NodePickResult* pick_result(const std::function<bool(const Scene::NodePickResult&)>& predicate) const
    {
        auto it = std::find_if(m_pick_results.begin(), m_pick_results.end(), predicate);
        return it == m_pick_results.end() ? nullptr : &*it;
    }

    template <typename T>
    const Scene::NodePickResult* pick_result_with_tag_of_type() const
    {
        return pick_result([](const auto& pr) { return pr.node->template has_tag_of_type<T>(); });
    }

    template <typename T>
    const Scene::Node* pick_result_node_with_tag_of_type() const
    {
        auto* r = pick_result_with_tag_of_type<T>();
        return r ? r->node : nullptr;
    }

private:
    Platform::MouseEvent m_mouse_event;
    Scene::Ray m_pick_ray;
    Scene::NodePickResults m_pick_results;
    Render::ScreenInfo m_screen_info;
};

}
