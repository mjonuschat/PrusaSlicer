#pragma once

#include <utility>

#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Platform/MouseEvent.hpp"

namespace Slic3r::App::Scene {

class GizmoEventContext {
public:
    GizmoEventContext(
        const Platform::MouseEvent& mouse_event,
        Ray  pick_ray,
        NodePickResults  pick_results,
        const Render::ScreenInfo& screen_info
    )
        : m_mouse_event(mouse_event)
        , m_pick_ray(std::move(pick_ray))
        , m_pick_results(std::move(pick_results))
        , m_screen_info(screen_info)
    {}

    const Platform::MouseEvent& mouse_event() const { return m_mouse_event; }
    const Ray& pick_ray() const { return m_pick_ray; }
    const NodePickResults& pick_results() const { return m_pick_results; }
    const Render::ScreenInfo& screen_info() const { return m_screen_info; }

    float screen_mouse_x() const { return m_screen_info.mouse_to_screen(m_mouse_event.x()); }
    float screen_mouse_y() const { return m_screen_info.mouse_to_screen(m_mouse_event.y()); }

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
private:
    Platform::MouseEvent m_mouse_event;
    Ray m_pick_ray;
    NodePickResults m_pick_results;
    Render::ScreenInfo m_screen_info;
};

} // namespace Slic3r::App::Scene
