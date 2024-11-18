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
class Scene;

class NodeBuilder {
public:
    explicit NodeBuilder(Scene& scene) : m_scene(scene), m_current(std::make_unique<Node>()) {}
    NodeBuilder& transform(const std::function<void(Transform3d&)>& modifier);
    NodeBuilder& set_mesh(const Render::Geometry* geometry, const Material& material, int layer_index=0);
    NodeBuilder& set_material_override(const Material& material);
    NodeBuilder& set_imgui_func(const FuncImguiRenderNodeComponent::RenderFunc& imgui_render_func);

    template <typename ImguiRendererT, typename ... ArgsT>
    NodeBuilder& set_imgui(ArgsT&&... args)
    {
        ensure_current();

        m_current->set_imgui_render_component(std::make_unique<ImguiRendererT>(args...));
        return *this;
    }

    NodeBuilder& set_aabb(const AABBMesh* aabb);
    NodeBuilder& set_aabb(const AABBMesh& aabb) { return set_aabb(&aabb); }

    template <typename T>
    NodeBuilder& set_tag(const T& tag_value)
    {
        ensure_current();
        m_current->set_tag(tag_value);
        return *this;
    }

    template <typename T, typename ... Args>
    NodeBuilder& set_transform_modifier(Args&& ... args)
    {
        ensure_current();
        m_current->set_transform_modifier(std::make_unique<T>(args...));
        return *this;
    }

    NodeBuilder& set_screen_space_sized_modifier(float scale);

    NodeBuilder& child(const std::function<void(NodeBuilder&)>& builder);
    NodeBuilder& children(size_t num_children, const std::function<void(NodeBuilder&, size_t)>& builder);

    template<typename ContainerT>
    NodeBuilder& child_for_each(
        typename ContainerT::const_iterator first,
        typename ContainerT::const_iterator last,
        const std::function<void(NodeBuilder&, const typename ContainerT::value_type&)>& builder
    )
    {
        std::for_each(first, last, [&](const typename ContainerT::value_type& element) {
            begin_child();
            builder(*this, element);
            end_child();
        });
        return *this;
    }

    template <typename ContainerT>
    NodeBuilder& child_for_each(const ContainerT& container, const std::function<void(NodeBuilder&, const typename ContainerT::value_type&)>& builder)
    {
        return child_for_each<ContainerT>(container.cbegin(), container.cend(), builder);
    }

    std::unique_ptr<Node> build();

private:
    NodeBuilder& begin_child();
    NodeBuilder& end_child();
    void ensure_current();
private:
    Scene& m_scene;
    std::unique_ptr<Node> m_current;
    Node::NodeOwningList m_parents;
};

}
