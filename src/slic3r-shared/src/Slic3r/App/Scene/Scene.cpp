#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/Material.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"

namespace Slic3r::App::Scene {

Material resolve_material(const Node& n)
{
    std::list<const Node*> path = {&n};
    const Node* p = n.parent();
    while (p) {
        path.push_front(p);
        p = p->parent();
    }

    Material mat;

    for (const Node* component : path) {
        if (const Material* override = component->material_override())
            mat.update(*override);

    }

    return mat;
}

void Scene::render(Render::CommandBuffer& cmd_buffer) const
{
    Node::ConstNodeList nodes;
    m_root.query([](auto n){ return n->has_render_component();}, nodes);
    std::stable_sort(nodes.begin(), nodes.end(), [](auto a, auto b) {
        return a->render_component()->layer_index() < b->render_component()->layer_index();
    });
    cmd_buffer.set_depth_test_enabled(true);
    for (const auto* n : nodes) {
        auto mat_override = resolve_material(*n);
        n->render_component()->render(*n, m_camera, mat_override, cmd_buffer);
    }
}

bool Scene::pick_at(float mouse_x, float mouse_y, ConstNodePickResults& results) const
{
    Node::ConstNodeList query_result;
    auto ray = m_camera.ray_at(mouse_x, mouse_y);
    const Node& n = m_root;
    auto ret = visit_conditional_transform<double>(n, [&ray](const Node& n, double& t) {
        if (!n.has_raycast_component())
            return false;
        return n.raycast_component()->raycast(n.world_transform(), ray.origin, ray.direction, t);
    });
    results.reserve(results.size() + ret.size());
    std::transform(
        ret.begin(), ret.end(), std::back_inserter(results),
        [](const auto& p) -> ConstNodePickResult { return {p.first, p.second}; }
    );
    std::sort(ret.begin(), ret.end(), [](auto a, auto b) {
        return a.second < b.second;
    });
    return !ret.empty();
}

bool Scene::pick_at(float mouse_x, float mouse_y, NodePickResults& results)
{
    Node::NodeList query_result;
    auto ray = m_camera.ray_at(mouse_x, mouse_y);
    Node& n = m_root;
    auto ret = visit_conditional_transform<double>(n, [&ray](Node& n, double& t) {
        if (!n.has_raycast_component())
            return false;
        return n.raycast_component()->raycast(n.world_transform(), ray.origin, ray.direction, t);
    });
    results.reserve(results.size() + ret.size());
    std::transform(
        ret.begin(), ret.end(), std::back_inserter(results),
        [](const auto& p) -> NodePickResult { return {p.first, p.second}; }
    );

    std::sort(ret.begin(), ret.end(), [](auto a, auto b) {
        return a.second < b.second;
    });
    return !ret.empty();
}
}
