//
// Created by Jan Bartipan on 01.03.2024.
//

#pragma once

#include "libslic3r/Color.hpp"
#include "slic3r/GUI/GLShader.hpp"

#include <vector>
#include <boost/variant/variant.hpp>
#include "libslic3r/Geometry.hpp"

namespace Slic3r::App::Scene {

using Transform = Eigen::Matrix4f;


struct ModelInfo
{
    size_t object_id {0};
    size_t volume_id {0};
    size_t instance_id {0};
};

struct GizmoInfo
{
    size_t gizmo_id;
};

struct NodeInfo
{
    std::optional<ModelInfo> model_info;
    std::optional<GizmoInfo> gizmo_info;
};

struct Mesh
{

};

struct Material
{
    using UniformValue = boost::variant<
        int,
        bool,
        float,
        const std::array<int, 2>,
        const std::array<int, 3>,
        const std::array<int, 4>,
        const std::array<float, 2>,
        const std::array<float, 3>,
        const std::array<float, 4>,
        const Transform3f,
        const Matrix3f,
        const Matrix4f,
        const Vec2f,
        const Vec3f,
        const ColorRGB,
        const ColorRGBA
    >;

    ColorRGBA render_color;
    GLShaderProgram* shader;
    virtual ~Material() = default;
    virtual void set_uniforms() = 0;
};


struct Node
{
    using NodeOwningList = std::vector<std::unique_ptr<Node>>;
    using NodeList = std::vector<Node*>;
    using ConstNodeList = std::vector<const Node*>;
    using NodePredicate = std::function<bool(const Node *)>;

    const Transform& local_transform() const { return m_local_xform; }
    const Transform& world_transform() const;

    void set_local_transform(const Matrix4f& t) {
        m_local_xform = t;
        mark_world_transform_dirty();
    }

    void set_world_transform(const Matrix4f& t) {
        m_world_xform = t;
        m_world_xform_dirty = false;
    }

    void add_child(Node* n) {
        if (n->m_parent == this)
            // already added
            return;

        n->m_parent = this;
        n->mark_world_transform_dirty();
        m_children.emplace_back(n);
    }

    bool remove_children(const NodePredicate &predicate) {
        return std::remove_if(
                   m_children.begin(), m_children.end(),
                   [predicate](NodeOwningList::const_reference node) {
                       return predicate(node.get());
                   }
               ) != m_children.end();
    }

    void query(const NodePredicate &predicate, NodeList &found_nodes);
    void query(const NodePredicate &predicate, ConstNodeList &found_nodes) const;

private:
    void mark_world_transform_dirty() const {
        if (!m_world_xform_dirty) {
            m_world_xform_dirty = true;
            mark_children_world_transform_dirty();
        }
    }
    // NOLINTBEGIN(misc-no-recursion): Mark recursion in query() as resolved
    void mark_children_world_transform_dirty() const {
        for (auto& ch: m_children) {
            ch->m_world_xform_dirty = true;
            ch->mark_children_world_transform_dirty();
        }
    }
    // NOLINTEND(misc-no-recursion)

private:
    Node* m_parent {nullptr};
    NodeOwningList m_children;

    Transform m_local_xform;
    mutable Transform m_world_xform;
    mutable bool m_world_xform_dirty {false};

    NodeInfo node_info;
};


class Scene {
public:
    Node& root() { return m_root; }
    const Node& root() const { return m_root; }
private:
    Node m_root;

};


}

