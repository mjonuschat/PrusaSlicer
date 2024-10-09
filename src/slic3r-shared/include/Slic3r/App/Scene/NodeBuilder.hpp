#pragma once

#include <functional>

#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Render {
class Geometery;
}

namespace Slic3r {
class AABBMesh;
}

namespace Slic3r::App::Scene {

class MeshRenderNodeComponent;
class Material;

class NodeBuilder {
public:
    NodeBuilder() : m_current(std::make_unique<Node>()) {}
    NodeBuilder& transform(const std::function<void(Transform3f&)>& modifier);
    NodeBuilder& set_mesh(const Render::Geometry* geometry, const Material& material);
    NodeBuilder& set_material_override(const Material& material);
    NodeBuilder& set_aabb(const AABBMesh* aabb);
    NodeBuilder& child(const std::function<void(NodeBuilder&)>& builder);
    NodeBuilder& children(size_t num_children, const std::function<void(NodeBuilder&, size_t)>& builder);

    template <typename I, typename T>
    NodeBuilder& child_for_each(I first, I last, const std::function<void(NodeBuilder&, const T&)>& builder)
    {
        std::for_each(first, last, [&](const T& element) {
            begin_child();
            builder(*this, element);
            end_child();
        });
    }

    std::unique_ptr<Node> build();

private:
    NodeBuilder& begin_child();
    NodeBuilder& end_child();
    void ensure_current();
private:
    std::unique_ptr<Node> m_current;
    Node::NodeOwningList m_parents;
};

}
