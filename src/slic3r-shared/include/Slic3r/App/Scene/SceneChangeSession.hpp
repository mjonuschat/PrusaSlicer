#pragma once

#include <vector>
#include <memory>

#include "Slic3r/App/Scene/Scene.hpp"

namespace Slic3r::App::Scene {

class ISceneChange
{
public:
    virtual ~ISceneChange() = default;

    virtual void roll_back(Scene& scene) = 0;
};

class AddNodeChange : public ISceneChange
{
public:
    explicit AddNodeChange(size_t node_id) : m_node_id(node_id) {}

    void roll_back(Scene& scene) override
    {
        auto* node = scene.node(m_node_id);
        scene.remove_children([&](const auto* n) { return n->id() == m_node_id; }, node->parent());
    }

private:
    size_t m_node_id;
};

class AddMaterialOverrideChange : public ISceneChange
{
public:
    explicit AddMaterialOverrideChange(size_t node_id, const Material* original = nullptr) 
    : m_node_id(node_id)
    {
        if (original)
            m_original_material = std::make_unique<Material>(*original);
    }

    void roll_back(Scene& scene) override 
    {
        Node* node = scene.node(m_node_id);
        if (m_original_material)
            node->set_material_override(*m_original_material);
        else
            node->remove_material_override();
    }

private:
    size_t m_node_id;
    std::unique_ptr<Material> m_original_material;
};

class SceneChangeSession final
{
public:
    class NodeChangeBuilder
    {
    public:
        NodeChangeBuilder(SceneChangeSession& session, Node& node): m_session(session), m_node(node) {}

        NodeChangeBuilder& add_child(Node* child)
        {
            m_session.m_scene.add_child(child, &m_node);
            m_session.m_changes.push_back(std::make_unique<AddNodeChange>(child->id()));
            return *this;
        }


        NodeChangeBuilder& set_material_override(const Material& m)
        {
            m_session.m_changes.push_back(std::make_unique<AddMaterialOverrideChange>(m_node.id(), m_node.material_override()));
            m_node.set_material_override(m);
            return *this;
        }

        SceneChangeSession& done()
        {
            return m_session;
        }
    private:
        SceneChangeSession& m_session;
        Node& m_node;
    };


    explicit SceneChangeSession(Scene& scene) : m_scene(scene) {}

    NodeChangeBuilder change(Node& n) { return {*this, n}; }

    void roll_back()
    {
        for(auto& c : m_changes)
            c->roll_back(m_scene);
        m_changes.clear();
    }

private:
    friend class NodeChangeBuilder;
    using SceneChangePtr = std::unique_ptr<ISceneChange>;
    using SceneChanges = std::vector<SceneChangePtr>;

    Scene& m_scene;
    SceneChanges m_changes;
};

} // namespace Slic3r::App::Scene
