#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/Material.hpp"
#include "Slic3r/App/Scene/MeshRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"

#include <imgui/imgui.h>

namespace Slic3r::App::Scene {

void MinimalSceneRenderCustomizer::on_render_begin(Render::CommandBuffer& cmd_buf)
{
    //cmd_buf.clear_buffers(true, true);
}

void MinimalSceneRenderCustomizer::on_opaque_pass_begin(Render::CommandBuffer& cmd_buf)
{
    cmd_buf.set_blending_enabled(false);
    cmd_buf.set_depth_write_enabled(true);
    cmd_buf.set_depth_test_enabled(true);
    cmd_buf.set_cull_face_enabled(true);
}


void MinimalSceneRenderCustomizer::on_transparent_pass_begin(Render::CommandBuffer& cmd_buf)
{
    cmd_buf.set_depth_test_enabled(true);
    cmd_buf.set_blending_enabled(true);
    Render::Blending blending { {Render::BlendFactor::SrcAlpha, Render::BlendFactor::OneMinusSrcAlpha}};
    cmd_buf.set_blending(blending);
    cmd_buf.set_depth_write_enabled(false);
    cmd_buf.set_cull_face_enabled(true);
}

void MinimalSceneRenderCustomizer::on_transparent_pass_end(Render::CommandBuffer& cmd_buf)
{
    cmd_buf.set_blending_enabled(false);
    cmd_buf.set_depth_write_enabled(true);
}



MinimalSceneRenderCustomizer Scene::ms_default_customizer;


Scene::Scene() : m_camera_trackball(m_camera)
{
    m_nodes_by_id[m_root.id()] = &m_root;
    m_root.set_debug_name("root");
}

void Scene::add_child(Node* node, Node* parent)
{
    register_node(node);
    if (parent == nullptr)
        parent = &m_root;
    parent->add_child(node);
}

bool Scene::remove_children(const Node::NodePredicate& predicate, Node* parent)
{
    if (parent == nullptr)
        parent = &m_root;

    return parent->remove_children([this, predicate](Node* n) {
        if (predicate(n)) {
            unregister_node(n);
            return true;
        }
        return false;
    });
}

Node::NodeOwningList Scene::detach_children(const Node::NodePredicate & predicate, Node* parent)
{
    if (parent == nullptr)
        parent = &m_root;

    return parent->detach_children([&](auto n){
        if (predicate(n)) {
            unregister_node(n);
            return true;
        }
        return false;
    });
}

void Scene::register_node(Node* node)
{
    visit(*node, [this](Node& n) {
        m_nodes_by_id[n.id()] = &n;

        auto* modifier = n.transform_modifier();
        if (modifier == nullptr)
            return;
        auto* cam_listener = dynamic_cast<ICameraUpdateListener*>(modifier);
        if (cam_listener)
            m_camera.add_update_listener(cam_listener);
    });
}

void Scene::unregister_node(Node* node)
{
    visit(*node, [this](Node& n) {
        m_nodes_by_id.erase(n.id());
        auto* modifier = n.transform_modifier();
        if (modifier == nullptr)
            return;
        auto* cam_listener = dynamic_cast<ICameraUpdateListener*>(modifier);
        if (cam_listener)
            m_camera.remove_update_listener(cam_listener);
    });
}


Render::Material resolve_material(const Node& n)
{
    std::list<const Node*> path = {&n};
    const Node* p = n.parent();
    while (p) {
        path.push_front(p);
        p = p->parent();
    }

    Render::Material mat;

    const IRenderNodeComponent* render_component = n.render_component();
    if (render_component != nullptr)
        mat = render_component->material();
    for (const Node* component : path) {
        if (const Render::Material* override = component->material_override())
            mat.update(*override);
    }




    return mat;
}

void Scene::render(Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer) const
{
    Node::ConstNodeList nodes;
    m_root.query([](auto n){ return n->has_render_component();}, nodes);

    using NodeMaterial = std::pair<const Node*, Render::Material>;
    std::vector<NodeMaterial> nodes_with_materials;
    nodes_with_materials.reserve(nodes.size());
    for (const Node* node : nodes)
        nodes_with_materials.emplace_back(node, resolve_material(*node));

    std::stable_partition(nodes_with_materials.begin(), nodes_with_materials.end(), [](const auto& p) {
        return !p.second.transparent();
    });
    std::stable_sort(nodes_with_materials.begin(), nodes_with_materials.end(), [](const auto& a, const auto& b) {
        return a.first->render_component()->layer_index() < b.first->render_component()->layer_index();
    });

    cmd_buffer.set_depth_test_enabled(true);

    if (customizer)
        customizer->on_render_begin(cmd_buffer);

    constexpr int INITIAL_LAYER = std::numeric_limits<int>::min();
    int last_layer = INITIAL_LAYER;
    bool was_opaque = false;
    for (const auto& [n, mat]  : nodes_with_materials) {
        const bool first_iteration = last_layer == INITIAL_LAYER;

        // did we start next layer
        if (auto layer = n->render_component()->layer_index(); layer != last_layer) {
            //cmd_buffer.clear_buffers(false, true);
            if (customizer) {
                if (last_layer != INITIAL_LAYER)
                    customizer->on_layer_end(cmd_buffer, last_layer);
                customizer->on_layer_begin(cmd_buffer, layer);
            }
            last_layer = layer;
        }

        // did we switch between opaque/transparent passes
        bool is_opaque = !mat.transparent();
        if (customizer) {
            bool pass_switch = is_opaque != was_opaque;

            // end last pass
            if (!first_iteration && pass_switch) {
                if (was_opaque)
                    customizer->on_opaque_pass_end(cmd_buffer);
                else
                    customizer->on_transparent_pass_end(cmd_buffer);
            }

            // begin new pass
            if (pass_switch) {
                if (is_opaque)
                    customizer->on_opaque_pass_begin(cmd_buffer);
                else
                    customizer->on_transparent_pass_begin(cmd_buffer);
            }
        }

        n->render_component()->render(*n, m_camera, mat, cmd_buffer);
        was_opaque = is_opaque;
    }

    if (customizer) {
        if (was_opaque)
            customizer->on_opaque_pass_end(cmd_buffer);
        else
            customizer->on_transparent_pass_end(cmd_buffer);
        customizer->on_layer_end(cmd_buffer, last_layer);
        customizer->on_render_end(cmd_buffer);
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
            return raycast.projected_bounding_box(mvp.cast<float>(), cam.viewport());
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

bool Scene::pick_at(float mouse_x, float mouse_y, ConstNodePickResults& results, Ray* out_ray) const
{
    Node::ConstNodeList query_result;
    auto ray = m_camera.ray_at(mouse_x, mouse_y);
    if (out_ray != nullptr)
        *out_ray = ray;
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

bool Scene::pick_at(float mouse_x, float mouse_y, NodePickResults& results, Ray* out_ray)
{
    Node::NodeList query_result;
    auto ray = m_camera.ray_at(mouse_x, mouse_y);
    if (out_ray != nullptr)
        *out_ray = ray;
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

void Scene::log_nodes() const
{
    std::string indent;
    indent.reserve(32);

    visit(m_root, [&](const Node& n) {
        indent.clear();
        const size_t level = n.level();
        for (size_t i = 0; i < level; i++)
            indent.append(" ");
        SPDLOG_INFO("{}- {}", indent, n.debug_name());
        indent.append("  ");
        SPDLOG_INFO("{}world transform:", indent);
        const auto& wt = n.world_transform();
        for (size_t i = 0; i < 4; i++) {
            SPDLOG_INFO("{}  ({:5} {:5} {:5} {:5})", indent, wt(i, 0), wt(i, 1), wt(i, 2), wt(i, 3));
        }
    });
}

}
