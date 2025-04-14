#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/Material.hpp"
#include "Slic3r/App/Render/ScopedDebugGroup.hpp"
#include "Slic3r/App/Render/MathUtils.hpp"
#include "Slic3r/App/Render/Framebuffer.hpp"
#include "Slic3r/App/Render/FramebufferManager.hpp"
#include "Slic3r/App/Scene/MeshRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"

#include <imgui/imgui.h>

namespace Slic3r::App::Scene {

void MinimalSceneRenderCustomizer::on_render_begin(Render::CommandBuffer& cmd_buf)
{
    //cmd_buf.clear_buffers(true, true);
}

void MinimalSceneRenderCustomizer::on_opaque_pass_begin(
    Render::CommandBuffer& cmd_buf, size_t layer_index
)
{
    cmd_buf.set_blending_enabled(false);
    cmd_buf.set_depth_write_enabled(true);
    cmd_buf.set_depth_test_enabled(true);
    cmd_buf.set_cull_face_enabled(true);
}


void MinimalSceneRenderCustomizer::on_transparent_pass_begin(
    Render::CommandBuffer& cmd_buf, size_t layer_index
)
{
    cmd_buf.set_depth_test_enabled(true);
    cmd_buf.set_blending_enabled(true);
    Render::Blending blending { {Render::BlendFactor::SrcAlpha, Render::BlendFactor::OneMinusSrcAlpha}};
    cmd_buf.set_blending(blending);
//    cmd_buf.set_depth_write_enabled(false);
    cmd_buf.set_cull_face_enabled(false);
}

void MinimalSceneRenderCustomizer::on_transparent_pass_end(
    Render::CommandBuffer& cmd_buf, size_t layer_index
)
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
            m_camera.add_listener<ICameraUpdateListener>(cam_listener);
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
            m_camera.remove_listener<ICameraUpdateListener>(cam_listener);
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

Scene::NodeMaterials Scene::collect_nodes_with_material(const Node::NodePredicate& predicate) const
{
    Scene::NodeMaterials ret;
    Node::ConstNodeList nodes;
    m_root.query(predicate, nodes);
    if (!nodes.empty()) {
        ret.reserve(nodes.size());
        for (const Node* node : nodes)
            ret.emplace_back(node, resolve_material(*node));
    }
    return ret;
}

void Scene::render_shadowsmap_pass(Render::Device& device) const
{
    // WARNING
    // assumes cull face mode == Render::CullFaceMode::Back

    if (m_shadows.shadowsmap_framebuffer == nullptr)
        m_shadows.pending_shadowsmap_size = m_shadows.DEFAULT_SHADOWSMAP_SIZE;

    if (m_shadows.pending_shadowsmap_size.has_value() && *m_shadows.pending_shadowsmap_size != m_shadows.shadowsmap_size) {
        if (m_shadows.shadowsmap_framebuffer != nullptr)
            device.context().framebuffer_manager().destroy(m_shadows.shadowsmap_framebuffer);
        Render::FramebufferCreationData data;
        data.width = *m_shadows.pending_shadowsmap_size;
        data.height = *m_shadows.pending_shadowsmap_size;
        m_shadows.shadowsmap_framebuffer = device.context().framebuffer_manager().create(data);
        m_shadows.shadowsmap_size = *m_shadows.pending_shadowsmap_size;
        m_shadows.pending_shadowsmap_size.reset();
    }

    auto cmd_buffer = device.create_command_buffer();
    // set light viewport
    cmd_buffer->set_viewport({ 0, 0, m_shadows.shadowsmap_size, m_shadows.shadowsmap_size });
    cmd_buffer->set_depth_test_enabled(true);
    cmd_buffer->set_cull_face_mode(Render::CullFaceMode::Front);
    cmd_buffer->set_cull_face_enabled(true);
    cmd_buffer->bind_framebuffer(*m_shadows.shadowsmap_framebuffer);
    cmd_buffer->clear_buffers(false, true);

    Eigen::AlignedBox3d world_aabb = m_shadows.bed_aabb;

    Node::ConstNodeList nodes;
    m_root.query([](auto n) {
        return n->has_render_component() && n->render_component()->cast_shadows();
    }, nodes);

    if (!nodes.empty()) {
        for (const Node* node : nodes) {
            if (node->has_raycast_component())
                world_aabb.extend(node->raycast_component()->world_bounding_box(node->world_transform()).cast<double>());
        }

        Vec3d center = world_aabb.center();

        Vec3d eye_light_dir = { 0.4574957, -0.4574957, -0.7624929 }; // taken from shader
        Vec4d world_light_dir_omo = m_camera.model() * Vec4d(eye_light_dir.x(), eye_light_dir.y(), eye_light_dir.z(), 0.0);
        Vec3d world_light_dir = Vec3d(world_light_dir_omo.x(), world_light_dir_omo.y(), world_light_dir_omo.z());
        Vec3d world_light_pos = center - 1.0f * world_light_dir;

        m_shadows.light_cam.look_at(world_light_pos, center, Vec3d::UnitZ());
        m_shadows.light_cam.switch_projection_type();

        Eigen::AlignedBox3d light_aabb = world_aabb.transformed(Transform3d(m_shadows.light_cam.view()));

        Vec3d eye_min = { DBL_MAX, DBL_MAX, DBL_MAX };
        Vec3d eye_max = { -DBL_MAX, -DBL_MAX, -DBL_MAX };
        Transform3d view = Transform3d(m_shadows.light_cam.view());
        for (int i = 0; i < 8; ++i) {
            Vec3d eye_c = light_aabb.corner(Eigen::AlignedBox3d::CornerType(i));
            eye_min.x() = std::min(eye_min.x(), eye_c.x());
            eye_min.y() = std::min(eye_min.y(), eye_c.y());
            eye_min.z() = std::min(eye_min.z(), -eye_c.z());
            eye_max.x() = std::max(eye_max.x(), eye_c.x());
            eye_max.y() = std::max(eye_max.y(), eye_c.y());
            eye_max.z() = std::max(eye_max.z(), -eye_c.z());
        }

        double delta_z = eye_min.z() - 10.0;
        world_light_pos += delta_z * world_light_dir;
        eye_min.z() -= delta_z;
        eye_max.z() -= delta_z;

        m_shadows.light_cam.look_at(world_light_pos, center, Vec3d::UnitZ());

        double max_x = std::max(std::abs(eye_min.x()), std::abs(eye_max.x()));
        double max_y = std::max(std::abs(eye_min.y()), std::abs(eye_max.y()));

        m_shadows.light_cam.set_projection(
            Render::ortho(-max_x, max_x, -max_y, max_y, eye_min.z(), eye_max.z()));

        Render::Material material = Render::Material{}
            .set_shader(device.context().shader_manager().shader("shadowsmap"));

        for (const Node* node : nodes) {
            node->render_component()->render(*node, m_shadows.light_cam, material, *cmd_buffer);
        }
    }

    cmd_buffer->unbind_framebuffer(*m_shadows.shadowsmap_framebuffer);
    cmd_buffer->set_cull_face_mode(Render::CullFaceMode::Back);
    // restore camera viewport
    cmd_buffer->set_viewport(m_camera.viewport());
}

void Scene::render_shadows_receivers_pass(Render::Device& device, Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer) const
{
    NodeMaterials nodes = collect_nodes_with_material([](auto n) {
        return n->has_render_component() && n->render_component()->receive_shadows() && !resolve_material(*n).transparent();
    });

    if (nodes.empty())
        return;

    for (auto& [node, material] : nodes) {
        Matrix4f light_matrix = (m_shadows.light_cam.projection() * m_shadows.light_cam.view() * node->world_transform()).cast<float>();
        std::string name = device.context().shader_manager().shader_name(material.shader());
        material
            .set_shader(device.context().shader_manager().shader(name + "_shadows"))
            .set_uniform("light_matrix", light_matrix)
            .set_uniform("shadowsmap", m_shadows.SHADOWSMAP_TEXTURE_UNIT);
    }

    cmd_buffer.bind_texture(m_shadows.SHADOWSMAP_TEXTURE_UNIT, *m_shadows.shadowsmap_framebuffer->depth());

    cmd_buffer.set_depth_test_enabled(true);

    if (customizer)
        customizer->on_render_begin(cmd_buffer);

    constexpr int INITIAL_LAYER = std::numeric_limits<int>::min();
    int current_layer = INITIAL_LAYER;
    for (const auto& [n, mat]  : nodes) {
        const bool first_iteration = current_layer == INITIAL_LAYER;

        // did we start next layer
        if (auto layer = n->render_component()->layer_index(); layer != current_layer) {
            if (customizer) {
                if (current_layer != INITIAL_LAYER)
                    customizer->on_layer_end(cmd_buffer, current_layer);
                customizer->on_layer_begin(cmd_buffer, layer);
            }
            current_layer = layer;
        }

        // did we switch between opaque/transparent passes
        if (customizer) {
            // end last pass
            if (!first_iteration)
                customizer->on_opaque_pass_end(cmd_buffer, current_layer);

            // begin new pass
            customizer->on_opaque_pass_begin(cmd_buffer, current_layer);
        }

        n->render_component()->render(*n, m_camera, mat, cmd_buffer);
    }

    if (customizer) {
        customizer->on_opaque_pass_end(cmd_buffer, current_layer);
        customizer->on_layer_end(cmd_buffer, current_layer);
        customizer->on_render_end(cmd_buffer);
    }

    cmd_buffer.unbind_texture(m_shadows.SHADOWSMAP_TEXTURE_UNIT, *m_shadows.shadowsmap_framebuffer->depth());
}

void Scene::render_no_shadows_pass(Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer) const
{
    NodeMaterials nodes = collect_nodes_with_material([this](auto n) {
        return n->has_render_component() && (!m_shadows.enabled || !n->render_component()->receive_shadows());
    });

    if (nodes.empty())
        return;

    std::stable_partition(nodes.begin(), nodes.end(), [](const auto& p) {
        return !p.second.transparent();
    });
    std::stable_sort(nodes.begin(), nodes.end(), [](const auto& a, const auto& b) {
        return a.first->render_component()->layer_index() < b.first->render_component()->layer_index();
    });

    cmd_buffer.set_depth_test_enabled(true);

    if (customizer)
        customizer->on_render_begin(cmd_buffer);

    constexpr int INITIAL_LAYER = std::numeric_limits<int>::min();
    int current_layer = INITIAL_LAYER;
    // set was_opaque as the opposite of the first element in list
    bool was_opaque = nodes.front().second.transparent();
    for (const auto& [n, mat]  : nodes) {
        const bool first_iteration = current_layer == INITIAL_LAYER;

        // did we start next layer
        if (auto layer = n->render_component()->layer_index(); layer != current_layer) {
            //cmd_buffer.clear_buffers(false, true);
            if (customizer) {
                if (current_layer != INITIAL_LAYER)
                    customizer->on_layer_end(cmd_buffer, current_layer);
                customizer->on_layer_begin(cmd_buffer, layer);
            }
            current_layer = layer;
        }

        // did we switch between opaque/transparent passes
        bool is_opaque = !mat.transparent();
        if (customizer) {
            bool pass_switch = is_opaque != was_opaque;

            // end last pass
            if (!first_iteration && pass_switch) {
                if (was_opaque)
                    customizer->on_opaque_pass_end(cmd_buffer, current_layer);
                else
                    customizer->on_transparent_pass_end(cmd_buffer, current_layer);
            }

            // begin new pass
            if (pass_switch) {
                if (is_opaque)
                    customizer->on_opaque_pass_begin(cmd_buffer, current_layer);
                else
                    customizer->on_transparent_pass_begin(cmd_buffer, current_layer);
            }
        }

        n->render_component()->render(*n, m_camera, mat, cmd_buffer);
        was_opaque = is_opaque;
    }

    if (customizer) {
        if (was_opaque)
            customizer->on_opaque_pass_end(cmd_buffer, current_layer);
        else
            customizer->on_transparent_pass_end(cmd_buffer, current_layer);
        customizer->on_layer_end(cmd_buffer, current_layer);
        customizer->on_render_end(cmd_buffer);
    }
}

void Scene::render(Render::Device& device, Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer,
    SceneRenderFlag flags) const
{
    Render::ScopedDebugGroup event_scene_render("Scene", cmd_buffer);

    bool shadows = (uint32_t(flags) & uint32_t(SceneRenderFlag::Shadows)) != 0;
    if (m_shadows.enabled && shadows) {
        render_shadowsmap_pass(device);
        render_shadows_receivers_pass(device, cmd_buffer, customizer);
    }
    render_no_shadows_pass(cmd_buffer, customizer);
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

            const auto vp = p * v;

            return projected_bounding_box(raycast, m, vp.cast<float>(), cam.viewport());
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
