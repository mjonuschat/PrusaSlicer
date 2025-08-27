#pragma once

#include "Slic3r/Domain/Types.hpp"

#define ENABLE_DEBUG_RENDER_SCENE_AABB 0

namespace Slic3r::App::Scene {
class Scene;
class ScenePresenterProjectContext;
class Camera;
class Node;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Plater {

class PlaterCameraFrustumUpdater
{
public:
    void update_scene_aabb(const Scene::Scene& scene);
    void update_camera_frustum(Scene::Camera& camera);
#if ENABLE_DEBUG_RENDER_SCENE_AABB
    void update_scene_aabb_node(Scene::ScenePresenterProjectContext& ctx, Render::Device& device);
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB

private:
    Eigen::AlignedBox3d m_scene_aabb;
#if ENABLE_DEBUG_RENDER_SCENE_AABB
    Scene::Node* m_scene_aabb_node{nullptr};
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB
};

} // namespace Slic3r::App::Plater
