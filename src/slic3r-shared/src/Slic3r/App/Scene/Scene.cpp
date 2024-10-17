#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/Material.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"

#include <imgui/imgui.h>

namespace Slic3r::App::Scene {


void Scene::add_child(Node* node, Node* parent)
{
    visit(*node, [this](Node& n) {
        auto* modifier = n.transform_modifier();
        if (modifier == nullptr)
            return;
        auto* cam_listener = dynamic_cast<ICameraUpdateListener*>(modifier);
        if (cam_listener)
            m_camera.add_update_listener(cam_listener);
    });

    if (parent == nullptr)
        parent = &m_root;
    parent->add_child(node);
}

bool Scene::remove_children(const Node::NodePredicate& predicate, Node* parent)
{
    if (parent == nullptr)
        parent = &m_root;

    return parent->remove_children(predicate, [this](Node* n) {
        visit(*n, [this](Node& n) {
            auto* modifier = n.transform_modifier();
            if (modifier == nullptr)
                return;
            auto* cam_listener = dynamic_cast<ICameraUpdateListener*>(modifier);
            if (cam_listener)
                m_camera.remove_update_listener(cam_listener);
        });
    });
}


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

Eigen::AlignedBox<float, 2> resolve_bounding_box(const Node& node, const Camera& cam)
{
    const Node* n = &node;
    while (n) {
        if (n->has_raycast_component()) {
            const auto& raycast = *n->raycast_component();
            const auto& m = n->world_transform();
            const auto v = cam.view();
            const auto& p = cam.projection();

            const auto mvp = p * v * m;
            return raycast.projected_bounding_box(mvp, cam.viewport());
        }
        n = n->parent();
    }

    return {};
}

Eigen::AlignedBox<float, 2> to_imgui_coords(const Eigen::AlignedBox<float, 2>& bb, const Render::ScreenInfo& screen_info)
{
    Eigen::AlignedBox<float, 2> ret;
    ret
        .extend(Vec2f{
            screen_info.physical_to_imgui_x(bb.min().x()),
            screen_info.physical_to_imgui_y(bb.min().y()),
        })
        .extend(Vec2f{
            screen_info.physical_to_imgui_x(bb.max().x()),
            screen_info.physical_to_imgui_y(bb.max().y()),
        });
    return ret;
}


void Scene::render_imgui(const Render::ScreenInfo& screen_info) const
{
    Node::ConstNodeList nodes;
    m_root.query([](auto n){ return n->has_imgui_render_component();}, nodes);
    for (const auto* n : nodes) {
        auto bb = resolve_bounding_box(*n, m_camera);
        if (!bb.isEmpty())
            n->imgui_render_component()->render_imgui(*n, to_imgui_coords(bb, screen_info));
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
        return n.raycast_component()->raycast(n.world_transform(), ray, t);
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
        return n.raycast_component()->raycast(n.world_transform(), ray, t);
    });
    std::sort(ret.begin(), ret.end(), [](auto a, auto b) {
        return a.second < b.second;
    });
    results.reserve(results.size() + ret.size());
    std::transform(
        ret.begin(), ret.end(), std::back_inserter(results),
        [](const auto& p) -> NodePickResult { return {p.first, p.second}; }
    );

    return !ret.empty();
}
}
