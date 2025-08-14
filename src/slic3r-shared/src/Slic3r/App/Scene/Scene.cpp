#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/Material.hpp"
#include "Slic3r/App/Render/ScopedDebugGroup.hpp"
#include "Slic3r/App/Render/MathUtils.hpp"
#include "Slic3r/App/Render/Framebuffer.hpp"
#include "Slic3r/App/Render/FramebufferManager.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Render/TextureManager.hpp"
#include "Slic3r/App/Scene/MeshRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/NodeVisitor.hpp"
#include "Slic3r/App/Scene/LightingHelper.hpp"
#include "Slic3r/App/LightSetting.hpp"

#include "Slic3r/Domain/Color.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <imgui/imgui.h>

#include <list>
#include <random>

using Slic3r::Domain::ColorRGBA;
using Slic3r::Domain::SquareMatrix4f;
using Slic3r::Domain::Transform3d;
using Slic3r::Domain::Vec3d;
using Slic3r::Domain::Vec2f;
using Slic3r::Domain::Vec3f;

namespace Slic3r::App::Scene {

enum class ShadingPass
{
    Shadowsmap,
    ShadowsReceivers,
    AOGBuffer,
};

std::vector<std::pair<std::string, std::string>> SHADOWSMAP_PASS_DICTIONARY = {
    {"gouraud_light", "shadowsmap"},
    {"gouraud_light_double_z_clip", "shadowsmap_double_z_clip"},
    {"printbed"     , "shadowsmap"},
    {"options"      , "options_shadowsmap"},
    {"segments"     , "segments_shadowsmap"},
};

std::vector<std::pair<std::string, std::string>> SHADOWS_RECEIVERS_PASS_DICTIONARY = {
    {"gouraud_light", "phong_shadows"},
    {"gouraud_light_double_z_clip", "phong_shadows_double_z_clip"},
    {"printbed"     , "printbed_phong_shadows"},
    {"options"      , "options_phong_shadows"},
    {"segments"     , "segments_phong_shadows"},
};

std::vector<std::pair<std::string, std::string>> AO_G_BUFFER_PASS_DICTIONARY = {
    {"gouraud_light", "gbuffer_ao"},
    {"gouraud_light_double_z_clip", "gbuffer_ao_double_z_clip"},
    {"printbed"     , "printbed_ao"},
    {"options"      , "options_ao"},
    {"segments"     , "segments_ao"},
};

static std::string shader_name_by_shading_pass(const std::string& shader_name, ShadingPass pass)
{
    switch (pass)
    {
    case ShadingPass::Shadowsmap:
    {
        auto it = std::find_if(SHADOWSMAP_PASS_DICTIONARY.begin(), SHADOWSMAP_PASS_DICTIONARY.end(),
          [&shader_name](const auto& pair) { return pair.first == shader_name; });
        return (it != SHADOWSMAP_PASS_DICTIONARY.end()) ? it->second : shader_name;
    }
    case ShadingPass::ShadowsReceivers:
    {
        auto it = std::find_if(SHADOWS_RECEIVERS_PASS_DICTIONARY.begin(), SHADOWS_RECEIVERS_PASS_DICTIONARY.end(),
          [&shader_name](const auto& pair) { return pair.first == shader_name; });
        return (it != SHADOWS_RECEIVERS_PASS_DICTIONARY.end()) ? it->second : shader_name;
    }
    case ShadingPass::AOGBuffer:
    {
        auto it = std::find_if(AO_G_BUFFER_PASS_DICTIONARY.begin(), AO_G_BUFFER_PASS_DICTIONARY.end(),
          [&shader_name](const auto& pair) { return pair.first == shader_name; });
        return (it != AO_G_BUFFER_PASS_DICTIONARY.end()) ? it->second : shader_name;
    }
    default:
    {
        // unsupported target
        PANIC("Unsupported shading pass");
    }
    }
}

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


GraphicsSettings Scene::s_graphics_settings;

MinimalSceneRenderCustomizer Scene::ms_default_customizer;


Scene::Scene() : m_camera_trackball(m_camera)
{
    m_nodes_by_id[m_root.id()] = &m_root;
    m_root.set_debug_name("root");

    if (m_pbr.enabled)
        set_ao_enabled(true);
    else if (m_ao.enabled)
        set_shadows_enabled(true);
    else
        validate_lights(m_lighting.lights);
}

void Scene::add_child(Node* node, Node* parent)
{
    register_node(node);
    if (parent == nullptr)
        parent = &m_root;
    parent->add_child(node);
}

bool Scene::remove_child(Node* node)
{
    auto* parent = node->parent();
    if (parent == nullptr)
        return false;
    return remove_children([node](const Node* child) {
        return child == node;
    }, parent);
}

bool Scene::remove_children(const Node::NodePredicate& predicate, Node* parent)
{
    if (parent == nullptr)
        parent = &m_root;

    return parent->remove_children([this, predicate](Node* n) {
        if (predicate(n)) {
            invoke_listeners<ISceneChangedListener>([n](auto* listener) {
                listener->on_node_removed(n);
            });
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

void Scene::init_screen_quad(Render::Device& device) const
{
    m_screen_quad = const_cast<Scene*>(this)->geometry_manager().get_or_create("screen_quad", [&device]() {
        std::vector<std::pair<Vec2f, Vec2f>> sq_vertices = {
            {{-1.0f,  1.0f}, {0.0f, 1.0f}},
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},
            {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
            {{ 1.0f, -1.0f}, {1.0f, 0.0f}}
        };

        Render::GeometryBuilder<Render::VertexP2T2> builder;
        builder.reserve(sq_vertices.size(), 0);
        for (const auto& v : sq_vertices) {
            builder.add_vertex({ v.first, v.second });
        }
        builder.add_draw_command({ Render::PrimitiveType::TriangleStrip, 0, sq_vertices.size(), Render::Material() });
        return builder.build(device);
    });
}

void Scene::generate_ao_kernel(Render::Device& device) const
{
    if (m_ao.kernel.empty())
        m_ao.pending_kernel_size = m_ao.DEFAULT_KERNEL_SIZE;

    if (m_ao.pending_kernel_size.has_value() && *m_ao.pending_kernel_size != m_ao.kernel.size()) {
        std::uniform_real_distribution<float> random_floats(0.0f, 1.0f); // generates random floats between 0.0 and 1.0
        std::default_random_engine generator;

        m_ao.kernel.clear();
        m_ao.kernel.reserve(*m_ao.pending_kernel_size);
        for (size_t i = 0; i < *m_ao.pending_kernel_size; ++i) {
            Vec3f sample(random_floats(generator) * 2.0f - 1.0f,
                         random_floats(generator) * 2.0f - 1.0f,
                         random_floats(generator));
            sample.normalize();
            sample *= random_floats(generator);
            float scale = float(i) / float(*m_ao.pending_kernel_size);
            // scale samples s.t. they're more aligned to center of kernel
            scale = 0.1f + scale * scale * 0.9f;
            sample *= scale;
            m_ao.kernel.emplace_back(sample);
        }

        m_ao.pending_kernel_size.reset();
    }
}

void Scene::generate_ao_noise(Render::Device& device) const
{
    if (m_ao.noise_size == 0)
        m_ao.pending_noise_size = m_ao.DEFAULT_NOISE_SIZE;

    if (m_ao.pending_noise_size.has_value() && *m_ao.pending_noise_size != m_ao.noise_size) {
        m_ao.noise_size = *m_ao.pending_noise_size;

        std::uniform_real_distribution<float> random_floats(0.0f, 1.0f); // generates random floats between 0.0 and 1.0
        std::default_random_engine generator;

        std::vector<Vec3f> noise_data;
        noise_data.reserve(m_ao.noise_size * m_ao.noise_size);
        for (size_t i = 0; i < m_ao.noise_size * m_ao.noise_size; ++i) {
            Vec3f noise(random_floats(generator) * 2.0f - 1.0f,
                        random_floats(generator) * 2.0f - 1.0f,
                        0.0f); // rotate around z-axis (in tangent space)
            noise_data.emplace_back(noise);
        }

        m_ao.noise = device.context().texture_manager().get_or_create_dynamic("ao_noise", Domain::PixelFormat::RGB32F, m_ao.noise_size, m_ao.noise_size);
        m_ao.noise->set_data(Domain::PixelFormat::RGB32F, 0, m_ao.noise_size, m_ao.noise_size, (void*)noise_data.data());

        m_ao.pending_noise_size.reset();
    }
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

void Scene::render_background(Render::CommandBuffer& cmd_buffer, Render::Device& device, bool use_error_color) const
{
    static const ColorRGBA DEFAULT_BG_DARK_COLOR = {0.478f, 0.478f, 0.478f, 1.0f};
    static const ColorRGBA DEFAULT_BG_LIGHT_COLOR = {0.753f, 0.753f, 0.753f, 1.0f};
    static const ColorRGBA ERROR_BG_DARK_COLOR = {0.478f, 0.192f, 0.039f, 1.0f};
    static const ColorRGBA ERROR_BG_LIGHT_COLOR = {0.753f, 0.192f, 0.039f, 1.0f};

    const ColorRGBA top_color = use_error_color ? ERROR_BG_LIGHT_COLOR : DEFAULT_BG_LIGHT_COLOR;
    const ColorRGBA bottom_color = use_error_color ? ERROR_BG_DARK_COLOR : DEFAULT_BG_DARK_COLOR;

    Render::Material material;
    material
        .set_shader(device.context().shader_manager().shader("background"))
        .set_uniform("top_color", top_color)
        .set_uniform("bottom_color", bottom_color);

    cmd_buffer.bind_and_draw(*m_screen_quad, material);
    cmd_buffer.clear_buffers(false, true);
}

void Scene::render_shadowsmap_pass(Render::Device& device, ISceneRenderCustomizer* customizer) const
{
    if (m_shadows.framebuffer == nullptr)
        m_shadows.pending_framebuffer_size = m_shadows.DEFAULT_FRAMEBUFFER_SIZE;

    if (m_shadows.pending_framebuffer_size.has_value() && *m_shadows.pending_framebuffer_size != m_shadows.framebuffer_size) {
        if (m_shadows.framebuffer != nullptr)
            device.context().framebuffer_manager().destroy(m_shadows.framebuffer);
        Render::FramebufferCreationData data;
        data.width = *m_shadows.pending_framebuffer_size;
        data.height = *m_shadows.pending_framebuffer_size;
        m_shadows.framebuffer = device.context().framebuffer_manager().create(data);
        m_shadows.framebuffer_size = *m_shadows.pending_framebuffer_size;
        m_shadows.pending_framebuffer_size.reset();
    }

    auto cmd_buffer = device.create_command_buffer();
    // set light viewport
    cmd_buffer->bind_framebuffer(*m_shadows.framebuffer);
    cmd_buffer->set_viewport({ 0, 0, m_shadows.framebuffer_size, m_shadows.framebuffer_size });
    cmd_buffer->set_depth_test_enabled(true);
    cmd_buffer->set_cull_face_enabled(true);
    cmd_buffer->clear_buffers(false, true);

    Eigen::AlignedBox3d world_aabb = m_shadows.aabb;

    NodeMaterials nodes = collect_nodes_with_material([](auto n) {
        return n->has_render_component() && n->render_component()->cast_shadows() && !resolve_material(*n).transparent();
    });

    if (!nodes.empty()) {
        for (const auto& [node, material] : nodes) {
            if (node->has_raycast_component())
                world_aabb.extend(node->raycast_component()->world_bounding_box(node->world_transform()).cast<double>());
        }

        Vec3d center = world_aabb.center();

        auto it = std::find_if(m_lighting.lights.begin(), m_lighting.lights.end(),
            [](const Light& l) {
                return l.shadows;
            }
        );
        DEBUG_ASSERT(it != m_lighting.lights.end());

        Vec3d world_light_dir = (it->system == LightReferenceSystem::Camera) ?
            Vec3d(m_camera.model().block<3, 3>(0, 0) * it->direction.cast<double>()) :
            it->direction.cast<double>();
        Vec3d world_light_pos = center - 1.0f * world_light_dir;

        m_shadows.light_cam.look_at(world_light_pos, center, Vec3d::UnitZ());
        m_shadows.light_cam.switch_projection_type();

        Eigen::AlignedBox3d light_aabb = world_aabb.transformed(Transform3d(m_shadows.light_cam.view()));

        Vec3d eye_min = { DBL_MAX, DBL_MAX, DBL_MAX };
        Vec3d eye_max = { -DBL_MAX, -DBL_MAX, -DBL_MAX };
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

        if (customizer)
            customizer->on_render_begin(*cmd_buffer);

        constexpr int INITIAL_LAYER = std::numeric_limits<int>::min();
        int current_layer = INITIAL_LAYER;
        for (auto& [n, mat]  : nodes) {
            const bool first_iteration = current_layer == INITIAL_LAYER;

            // did we start next layer
            if (auto layer = n->render_component()->layer_index(); layer != current_layer) {
                if (customizer) {
                    if (current_layer != INITIAL_LAYER) {
                        customizer->on_opaque_pass_end(*cmd_buffer, current_layer);
                        customizer->on_layer_end(*cmd_buffer, current_layer);
                    }
                    customizer->on_layer_begin(*cmd_buffer, layer);
                    customizer->on_opaque_pass_begin(*cmd_buffer, layer);
                }
                current_layer = layer;
            }

            std::string shader_name = device.context().shader_manager().shader_name(mat.shader());
            shader_name = shader_name_by_shading_pass(shader_name, ShadingPass::Shadowsmap);
            mat
              .set_shader(device.context().shader_manager().shader(shader_name))
              .set_uniform("light_position", (Vec3f)world_light_pos.cast<float>());
            n->render_component()->render(*n, m_shadows.light_cam, m_lighting, mat, *cmd_buffer);
        }

        if (customizer) {
            customizer->on_opaque_pass_end(*cmd_buffer, current_layer);
            customizer->on_layer_end(*cmd_buffer, current_layer);
            customizer->on_render_end(*cmd_buffer);
        }
    }

    // restore camera viewport
    cmd_buffer->set_viewport(m_camera.viewport());
    cmd_buffer->unbind_framebuffer(*m_shadows.framebuffer);
}

void Scene::render_shadows_receivers_pass(Render::Device& device, Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer) const
{
    NodeMaterials nodes = collect_nodes_with_material([](auto n) {
        return n->has_render_component() && n->render_component()->receive_shadows() && !resolve_material(*n).transparent();
    });

    if (nodes.empty())
        return;

    Transform light_cam_proj_view_matrix = m_shadows.light_cam.projection() * m_shadows.light_cam.view();
    for (auto& [node, material] : nodes) {
        SquareMatrix4f light_cam_matrix = (light_cam_proj_view_matrix * node->world_transform()).cast<float>();
        std::string shader_name = device.context().shader_manager().shader_name(material.shader());
        shader_name = shader_name_by_shading_pass(shader_name, ShadingPass::ShadowsReceivers);
        material
            .set_shader(device.context().shader_manager().shader(shader_name))
            .set_uniform("light_matrix", light_cam_matrix)
            .set_uniform("shadows_intensity", m_shadows.intensity)
            .set_texture(m_shadows.SHADOWSMAP_TEX_UNIT, m_shadows.framebuffer->depth())
            .set_uniform("shadowsmap", m_shadows.SHADOWSMAP_TEX_UNIT);

        set_uniforms(m_lighting, material);
    }

    cmd_buffer.set_depth_test_enabled(true);
    cmd_buffer.set_cull_face_enabled(true);

    if (customizer)
        customizer->on_render_begin(cmd_buffer);

    constexpr int INITIAL_LAYER = std::numeric_limits<int>::min();
    int current_layer = INITIAL_LAYER;
    for (const auto& [n, mat]  : nodes) {
        const bool first_iteration = current_layer == INITIAL_LAYER;

        // did we start next layer
        if (auto layer = n->render_component()->layer_index(); layer != current_layer) {
            if (customizer) {
                if (current_layer != INITIAL_LAYER) {
                    customizer->on_opaque_pass_end(cmd_buffer, current_layer);
                    customizer->on_layer_end(cmd_buffer, current_layer);
                }
                customizer->on_layer_begin(cmd_buffer, layer);
                customizer->on_opaque_pass_begin(cmd_buffer, layer);
            }
            current_layer = layer;
        }

        n->render_component()->render(*n, m_camera, m_lighting, mat, cmd_buffer);
    }

    if (customizer) {
        customizer->on_opaque_pass_end(cmd_buffer, current_layer);
        customizer->on_layer_end(cmd_buffer, current_layer);
        customizer->on_render_end(cmd_buffer);
    }
}

void Scene::render_no_shadows_pass(Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer) const
{
    NodeMaterials nodes = collect_nodes_with_material([this](auto n) {
        return n->has_render_component() && ((!m_shadows.enabled && !m_ao.enabled)|| !n->render_component()->receive_shadows());
    });

    if (nodes.empty())
        return;

    std::stable_partition(nodes.begin(), nodes.end(), [](const auto& p) {
        return !p.second.transparent();
    });
    // Currently the only textured transparent object is the bed plate when the camera points upward or the bed is disabled.
    // Sorts transparent objects so that the bed plate is rendered last to avoid its update of the depth buffer
    // which would result in hiding the other transparent objects rendered after it
    auto first_transparent_it = std::find_if(nodes.begin(), nodes.end(), [](const auto& p) {
        return p.second.transparent();
    });
    if (first_transparent_it != nodes.end()) {
        std::stable_sort(first_transparent_it, nodes.end(), [](const auto& a, const auto& b) {
            return a.second.textures().size() < b.second.textures().size();
        });
    }
    // ensure that gizmo nodes are rendered last, as they require a cleanup of the depth buffer
    std::stable_sort(nodes.begin(), nodes.end(), [](const auto& a, const auto& b) {
        return a.first->render_component()->layer_index() < b.first->render_component()->layer_index();
    });    

    cmd_buffer.set_depth_test_enabled(true);
    cmd_buffer.set_cull_face_enabled(true);

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

        n->render_component()->render(*n, m_camera, m_lighting, mat, cmd_buffer);
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

void Scene::render_ao_gbuffer_pass(Render::Device& device, ISceneRenderCustomizer* customizer, const Domain::Index2& viewport_size) const
{
    if (m_ao.gbuffer_fb == nullptr || m_ao.framebuffer_size != viewport_size) {
        if (m_ao.gbuffer_fb != nullptr)
            device.context().framebuffer_manager().destroy(m_ao.gbuffer_fb);
        Render::FramebufferCreationData data;
        data.width = viewport_size[0];
        data.height = viewport_size[1];
        data.color_attachments.resize(5);
        data.color_attachments[AmbientOcclusion::EYE_POS_CLR_ATTR].format = Domain::PixelFormat::RGBA16F;
        data.color_attachments[AmbientOcclusion::LIGHT_POS_CLR_ATTR].format = Domain::PixelFormat::RGBA16F;
        data.color_attachments[AmbientOcclusion::EYE_NORM_CLR_ATTR].format = Domain::PixelFormat::RGBA16F;
        data.color_attachments[AmbientOcclusion::COLOR_CLR_ATTR].format = Domain::PixelFormat::RGBA8;
        data.color_attachments[AmbientOcclusion::PBR_MATERIAL_ATTR].format = Domain::PixelFormat::RGBA16F;
        m_ao.gbuffer_fb = device.context().framebuffer_manager().create(data);
    }

    auto cmd_buffer = device.create_command_buffer();
    cmd_buffer->bind_framebuffer(*m_ao.gbuffer_fb);
    cmd_buffer->set_depth_test_enabled(true);
    cmd_buffer->set_cull_face_enabled(true);
    cmd_buffer->set_viewport(m_camera.viewport());
    cmd_buffer->set_clear_values({ 0.0f, 0.0f, 0.0f, 0.0f });
    cmd_buffer->clear_buffers(true, true);

    NodeMaterials nodes = collect_nodes_with_material([](auto n) {
        return n->has_render_component() && n->render_component()->receive_shadows() && !resolve_material(*n).transparent();
    });

    if (!nodes.empty()) {
        Transform light_cam_proj_view_matrix = m_shadows.light_cam.projection() * m_shadows.light_cam.view();

        if (customizer)
            customizer->on_render_begin(*cmd_buffer);

        constexpr int INITIAL_LAYER = std::numeric_limits<int>::min();
        int current_layer = INITIAL_LAYER;
        for (auto& [n, mat]  : nodes) {
            const bool first_iteration = current_layer == INITIAL_LAYER;

            // did we start next layer
            if (auto layer = n->render_component()->layer_index(); layer != current_layer) {
                if (customizer) {
                    if (current_layer != INITIAL_LAYER) {
                        customizer->on_opaque_pass_end(*cmd_buffer, current_layer);
                        customizer->on_layer_end(*cmd_buffer, current_layer);
                    }
                    customizer->on_layer_begin(*cmd_buffer, layer);
                    customizer->on_opaque_pass_begin(*cmd_buffer, layer);
                }
                current_layer = layer;
            }

            SquareMatrix4f light_cam_matrix = (light_cam_proj_view_matrix * n->world_transform()).cast<float>();
            std::string shader_name = device.context().shader_manager().shader_name(mat.shader());
            shader_name = shader_name_by_shading_pass(shader_name, ShadingPass::AOGBuffer);
            mat
                .set_shader(device.context().shader_manager().shader(shader_name))
                .set_uniform("light_matrix", light_cam_matrix);

            if (m_pbr.enabled && n->render_component()->has_pbr())
                set_uniforms(*n->render_component()->pbr(), mat);

            n->render_component()->render(*n, m_camera, m_lighting, mat, *cmd_buffer);
        }

        if (customizer) {
            customizer->on_opaque_pass_end(*cmd_buffer, current_layer);
            customizer->on_layer_end(*cmd_buffer, current_layer);
            customizer->on_render_end(*cmd_buffer);
        }
    }

    cmd_buffer->unbind_framebuffer(*m_ao.gbuffer_fb);
}

void Scene::render_ao_texture_pass(Render::Device& device, const Domain::Index2& viewport_size) const
{
    if (m_ao.ao_tex_fb == nullptr || m_ao.framebuffer_size != viewport_size) {
        if (m_ao.ao_tex_fb != nullptr)
            device.context().framebuffer_manager().destroy(m_ao.ao_tex_fb);
        Render::FramebufferCreationData data;
        data.width = viewport_size[0];
        data.height = viewport_size[1];
        data.depth = false;
        Render::FramebufferColorAttachment color;
        color.format = Domain::PixelFormat::R32F;
        data.color_attachments.push_back(color);
        m_ao.ao_tex_fb = device.context().framebuffer_manager().create(data);
    }

    auto cmd_buffer = device.create_command_buffer();
    cmd_buffer->bind_framebuffer(*m_ao.ao_tex_fb);
    cmd_buffer->set_viewport(m_camera.viewport());
    cmd_buffer->set_clear_values({ 0.0f, 0.0f, 0.0f, 0.0f });
    cmd_buffer->clear_buffers(true, false);

    Vec2f v_size = { float(viewport_size[0]), float(viewport_size[1]) };
    SquareMatrix4f projection = m_camera.projection().cast<float>();

    Render::Material material;
    material
        .set_shader(device.context().shader_manager().shader("ao_texture"))
        .set_uniform("kernel_size", int(m_ao.kernel.size()))
        .set_uniform("radius", m_ao.radius)
        .set_uniform("bias", m_ao.bias)
        .set_uniform("viewport_size", v_size)
        .set_uniform("projection_matrix", projection)
        .set_uniform("g_eye_position", AmbientOcclusion::EYE_POS_TEX_UNIT)
        .set_uniform("g_eye_normal", AmbientOcclusion::EYE_NORM_TEX_UNIT)
        .set_uniform("tex_noise", AmbientOcclusion::NOISE_TEX_UNIT)
        .set_texture(AmbientOcclusion::EYE_POS_TEX_UNIT, m_ao.gbuffer_fb->color_attachment(AmbientOcclusion::EYE_POS_CLR_ATTR))
        .set_texture(AmbientOcclusion::EYE_NORM_TEX_UNIT, m_ao.gbuffer_fb->color_attachment(AmbientOcclusion::EYE_NORM_CLR_ATTR))
        .set_texture(AmbientOcclusion::NOISE_TEX_UNIT, m_ao.noise);


    for (size_t i = 0; i < m_ao.kernel.size(); ++i) {
        std::string name = "kernel[" + std::to_string(i) + "]";
        material.set_uniform(name, m_ao.kernel[i]);
    }

    cmd_buffer->bind_and_draw(*m_screen_quad, material);

    cmd_buffer->unbind_framebuffer(*m_ao.ao_tex_fb);
}

void Scene::render_ao_texture_blur_pass(Render::Device& device, const Domain::Index2& viewport_size) const
{
    if (m_ao.blur_fb == nullptr || m_ao.framebuffer_size != viewport_size) {
        if (m_ao.blur_fb != nullptr)
            device.context().framebuffer_manager().destroy(m_ao.blur_fb);
        Render::FramebufferCreationData data;
        data.width = viewport_size[0];
        data.height = viewport_size[1];
        data.depth = false;
        Render::FramebufferColorAttachment color;
        color.format = Domain::PixelFormat::R32F;
        data.color_attachments.push_back(color);
        m_ao.blur_fb = device.context().framebuffer_manager().create(data);
    }

    auto cmd_buffer = device.create_command_buffer();
    cmd_buffer->bind_framebuffer(*m_ao.blur_fb);
    cmd_buffer->set_viewport(m_camera.viewport());
    cmd_buffer->set_clear_values({ 0.0f, 0.0f, 0.0f, 0.0f });
    cmd_buffer->clear_buffers(true, false);

    Render::Material material;
    material
        .set_shader(device.context().shader_manager().shader("ao_blur"))
        .set_uniform("in_tex", AmbientOcclusion::AO_TEX_UNIT)
        .set_uniform("filter_size", int(m_ao.blur_filter_size))
        .set_texture(AmbientOcclusion::AO_TEX_UNIT, m_ao.ao_tex_fb->color_attachment(0));

    cmd_buffer->bind_and_draw(*m_screen_quad, material);

    cmd_buffer->unbind_framebuffer(*m_ao.blur_fb);
}

void Scene::render_ao_lighting_pass(Render::CommandBuffer& cmd_buffer, Render::Device& device) const
{
    cmd_buffer.set_depth_test_enabled(false);
    cmd_buffer.set_depth_write_enabled(false);
    cmd_buffer.set_blending_enabled(true);
    Render::Blending blending{ {Render::BlendFactor::SrcAlpha, Render::BlendFactor::OneMinusSrcAlpha} };
    cmd_buffer.set_blending(blending);

    SquareMatrix4f view = camera().view().cast<float>();

    Render::Material material;
    material
        .set_shader(device.context().shader_manager().shader("ao_lighting"))
        .set_uniform("apply_pbr", m_pbr.enabled)
        .set_uniform("pbr_intensity", m_pbr.enabled ? m_pbr.intensity : 1.0f)
        .set_uniform("apply_shadows", m_shadows.enabled)
        .set_uniform("shadows_intensity", m_shadows.enabled ? m_shadows.intensity : 0.0f)
        .set_uniform("g_eye_position", AmbientOcclusion::EYE_POS_TEX_UNIT)
        .set_uniform("g_light_position", AmbientOcclusion::LIGHT_POS_TEX_UNIT)
        .set_uniform("g_eye_normal", AmbientOcclusion::EYE_NORM_TEX_UNIT)
        .set_uniform("g_color", AmbientOcclusion::COLOR_TEX_UNIT)
        .set_uniform("g_material", AmbientOcclusion::PBR_MATERIAL_TEX_UNIT)
        .set_uniform("ssao", AmbientOcclusion::AO_TEX_UNIT)
        .set_uniform("shadowsmap", Shadows::SHADOWSMAP_TEX_UNIT)
        .set_uniform("view_matrix", view)
        .set_texture(AmbientOcclusion::EYE_POS_TEX_UNIT, m_ao.gbuffer_fb->color_attachment(AmbientOcclusion::EYE_POS_CLR_ATTR))
        .set_texture(AmbientOcclusion::LIGHT_POS_TEX_UNIT, m_ao.gbuffer_fb->color_attachment(AmbientOcclusion::LIGHT_POS_CLR_ATTR))
        .set_texture(AmbientOcclusion::EYE_NORM_TEX_UNIT, m_ao.gbuffer_fb->color_attachment(AmbientOcclusion::EYE_NORM_CLR_ATTR))
        .set_texture(AmbientOcclusion::COLOR_TEX_UNIT, m_ao.gbuffer_fb->color_attachment(AmbientOcclusion::COLOR_CLR_ATTR))
        .set_texture(AmbientOcclusion::PBR_MATERIAL_TEX_UNIT, m_ao.gbuffer_fb->color_attachment(AmbientOcclusion::PBR_MATERIAL_ATTR))
        .set_texture(AmbientOcclusion::AO_TEX_UNIT, m_ao.blur_fb->color_attachment(0))
        .set_texture(Shadows::SHADOWSMAP_TEX_UNIT, m_shadows.framebuffer->depth());
 
    set_uniforms(m_lighting, material);

    cmd_buffer.bind_and_draw(*m_screen_quad, material);

    cmd_buffer.set_blending_enabled(false);
    cmd_buffer.set_depth_write_enabled(true);
    cmd_buffer.set_depth_test_enabled(true);
}

void Scene::render(Render::Device& device, Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer) const
{
    Render::ScopedDebugGroup event_scene_render("Scene", cmd_buffer);

    if (m_screen_quad == nullptr)
        init_screen_quad(device);

    if (m_background_enabled)
        render_background(cmd_buffer, device, false);

    if (m_shadows.enabled)
        render_shadowsmap_pass(device, customizer);

    if (m_ao.enabled) {
        const Render::Rect& viewport = m_camera.viewport();
        Domain::Index2 viewport_size = { viewport.width, viewport.height };
        render_ao_gbuffer_pass(device, customizer, viewport_size);
        generate_ao_kernel(device);
        generate_ao_noise(device);
        render_ao_texture_pass(device, viewport_size);
        render_ao_texture_blur_pass(device, viewport_size);
        render_ao_lighting_pass(cmd_buffer, device);
        cmd_buffer.blit_to_draw_framebuffer(*m_ao.gbuffer_fb, viewport.width, viewport.height,
            Render::BlitFramebufferMask::DepthBufferBit, Render::BlitFramebufferFilter::Nearest);
        m_ao.framebuffer_size = viewport_size;
    }
    else if (m_shadows.enabled)
        render_shadows_receivers_pass(device, cmd_buffer, customizer);

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

void Scene::validate_lights(Lights& lights)
{
    if (m_shadows.enabled) {
        // ensure one light is set to cast shadows
        int count = std::count_if(lights.begin(), lights.end(),
            [](const Light& l) {
                return l.shadows;
            }
        );
        if (count == 0)
            lights.front().shadows = true;
        else if (count > 1) {
            // keeps only the first light as casting shadows
            bool found = false;
            for (auto& l : lights) {
                if (l.shadows) {
                    found = true;
                    continue;
                }
                if (found)
                    l.shadows = false;
            }
        }
    }
    else
        // ensure no light is set to cast shadows
        std::for_each(lights.begin(), lights.end(), [](Light& l) { l.shadows = false; });

    // avoid shininess == 0.0, see: https://registry.khronos.org/OpenGL-Refpages/gl4/html/pow.xhtml
    std::for_each(lights.begin(), lights.end(),
        [](Light& l) { if (l.shininess == 0.0f) l.shininess = 0.001f; });
}

void Scene::set_shadows_enabled(bool enable)
{
    m_shadows.enabled = enable;
    if (!enable) {
        m_ao.enabled = false;
        m_pbr.enabled = false;
    }
    validate_lights(m_lighting.lights);
}

void Scene::set_ao_enabled(bool enable)
{
    m_ao.enabled = enable;
    if (enable)
        set_shadows_enabled(true);
    else
        m_pbr.enabled = false;
}

void Scene::set_pbr_enabled(bool enable)
{
    m_pbr.enabled = enable;
    if (enable) {
        m_ao.enabled = true;
        set_shadows_enabled(true);
    }
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
