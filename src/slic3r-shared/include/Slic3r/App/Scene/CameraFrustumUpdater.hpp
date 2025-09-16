#pragma once

#include "Slic3r/Domain/Types.hpp"

#define ENABLE_DEBUG_RENDER_SCENE_AABB 0

#if ENABLE_DEBUG_RENDER_SCENE_AABB
namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB

namespace Slic3r::App::Scene {

class Scene;
class Camera;
#if ENABLE_DEBUG_RENDER_SCENE_AABB
class ScenePresenterProjectContext;
class Node;
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB

class CameraFrustumUpdater
{
public:
    void update_scene_aabb(const Scene& scene);
    void update_camera_frustum(Camera& camera);
#if ENABLE_DEBUG_RENDER_SCENE_AABB
    void update_scene_aabb_node(ScenePresenterProjectContext& ctx, Render::Device& device);
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB

private:
    Eigen::AlignedBox3d m_scene_aabb;
#if ENABLE_DEBUG_RENDER_SCENE_AABB
    Node* m_scene_aabb_node{nullptr};
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB
};

} // namespace Slic3r::App::Scene
