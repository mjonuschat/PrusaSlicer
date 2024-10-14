#pragma once
#include "Slic3r/App/Scene/INodeTransformModifier.hpp"
#include "Slic3r/App/Scene/Camera.hpp"

namespace Slic3r::App::Scene {
class Node;

class ScreenSpaceSizedTransformModifier : public INodeTransformModifier, public ICameraUpdateListener {
public:
    ScreenSpaceSizedTransformModifier(const Camera& cam, Node& node, float scale=1)
        : m_camera(cam), m_node(node), m_preserved_scale(scale)
    {}

    void modify_world_transform(Transform& world_xform) override;
    void camera_updated(const Camera& cam) override;

private:
    const Camera& m_camera;
    Node& m_node;
    float m_preserved_scale;
};
}
