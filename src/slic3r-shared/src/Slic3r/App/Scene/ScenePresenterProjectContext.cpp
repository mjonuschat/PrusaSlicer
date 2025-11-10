#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"
#if ENABLE_DEBUG_RENDER_SCENE_AABB
#include "Slic3r/App/Scene/AABBNodeHelper.hpp"
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB

namespace Slic3r::App::Scene {

#if ENABLE_DEBUG_RENDER_SCENE_AABB
void ScenePresenterProjectContext::update_scene_aabb_node(Render::Device& device, const Eigen::AlignedBox3d& aabb)
{
    if (!m_scene_aabb_node.dirty)
        return;

    if (m_scene_aabb_node.node == nullptr) {
        NodeBuilder builder(*m_scene);
        build_aabb_node(builder, *this, device, "scene_aabb", 0, Domain::ColorRGB::YELLOW());
        m_scene_aabb_node.node = builder.build().release();
        m_scene->add_child(m_scene_aabb_node.node);
    }

    DEBUG_ASSERT(m_scene_aabb_node.node != nullptr);
    update_aabb_node(*m_scene_aabb_node.node, aabb);
    m_scene_aabb_node.dirty = false;
}
#endif // ENABLE_DEBUG_RENDER_SCENE_AABB

} // namespace Slic3r::App::Scene
