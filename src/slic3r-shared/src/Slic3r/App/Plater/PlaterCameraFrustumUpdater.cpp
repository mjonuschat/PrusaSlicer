#include "Slic3r/App/Plater/PlaterCameraFrustumUpdater.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/AuxiliaryElementId.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"
#include "Slic3r/Domain/Color.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"

namespace Slic3r::App::Plater {

void PlaterCameraFrustumUpdater::update_scene_aabb(const Scene::Scene& scene)
{
    Scene::Node::ConstNodeList nodes;
    scene.root().query([](const Scene::Node* n) { return n->has_raycast_component(); }, nodes);

    DEBUG_ASSERT(!nodes.empty());

    m_scene_aabb = Eigen::AlignedBox3d();
    for (const Scene::Node* n : nodes) {
        m_scene_aabb.extend(
            n->raycast_component()->world_bounding_box(n->world_transform().matrix()).cast<double>()
        );
    }
}

void PlaterCameraFrustumUpdater::update_camera_frustum(Scene::Camera& camera)
{
    DEBUG_ASSERT(!m_scene_aabb.isEmpty());

    Domain::Transform3d view = Domain::Transform3d(camera.view().matrix());
    double z_min             = DBL_MAX;
    double z_max             = -DBL_MAX;
    for (size_t i = 0; i < 8; ++i) {
        Domain::Vec3d v_eye = view * m_scene_aabb.corner(Eigen::AlignedBox3d::CornerType(i));
        z_min               = std::min(z_min, -v_eye.z());
        z_max               = std::max(z_max, -v_eye.z());
    }
    static constexpr double MARGIN = 1.0;
    camera.set_z_near_far(std::max(10.0, z_min - MARGIN), std::max(1000.0, z_max + MARGIN));
}

#if ENABLE_DEBUG_RENDER_SCENE_AABB
void PlaterCameraFrustumUpdater::update_scene_aabb_node(Scene::ScenePresenterProjectContext& ctx, Render::Device& device)
{
    Scene::Scene& scene = ctx.scene();

    Scene::AuxiliaryElementId id{Scene::AuxiliaryElementId::Type::Volume, INT_MAX};

    auto& geom_mgr = ctx.model_geometry_manager();
    if (m_scene_aabb_node != nullptr) {
        // if node exists, remove it from scene and its geometry from the geometry manager
        scene.remove_child(m_scene_aabb_node);
        geom_mgr.release(id);
    }

    std::vector<Domain::Vec3f> vertices;
    vertices.reserve(8);
    for (size_t i = 0; i < 8; ++i) {
        vertices.emplace_back(m_scene_aabb.corner(Eigen::AlignedBox3d::CornerType(i)).cast<float>());
    }

    std::vector<Domain::Vec3f> lines = {
        vertices[0], vertices[1], vertices[1], vertices[3], vertices[3], vertices[2],
        vertices[2], vertices[0], vertices[0], vertices[4], vertices[1], vertices[5],
        vertices[2], vertices[6], vertices[3], vertices[7], vertices[4], vertices[5],
        vertices[5], vertices[7], vertices[7], vertices[6], vertices[6], vertices[4]
    };

    const auto* geom =
        geom_mgr.get_or_create(id, [&]() { return Render::geometry_from_lines(device, lines); });

    Render::Material material;
    material.set_shader(device.context().shader_manager().shader("flat")).set_uniform("uniform_color", Domain::ColorRGBA::YELLOW());

    Scene::NodeBuilder builder(scene);
    builder.set_debug_name("scene_aabb").set_mesh(geom, material, int(PlaterSceneLayer::DocumentObjects));

    m_scene_aabb_node = builder.build().release();
    scene.add_child(m_scene_aabb_node);
}
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB

} // namespace Slic3r::App::Plater
