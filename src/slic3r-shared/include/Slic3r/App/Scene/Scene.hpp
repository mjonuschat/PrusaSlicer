#pragma once

#include <vector>
#include <boost/variant/variant.hpp>

#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Render/Shader.hpp"

#include "libslic3r/Color.hpp"
#include "libslic3r/Geometry.hpp"
#include "libslic3r/AABBMesh.hpp"


namespace Slic3r::App::Scene {

struct NodePickResult
{
    Node* node;
    double t;
};

struct ConstNodePickResult
{
    const Node* node;
    double t;
};

using NodePickResults = std::vector<NodePickResult>;
using ConstNodePickResults = std::vector<ConstNodePickResult>;

class Scene
{
public:
    Node& root() { return m_root; }
    const Node& root() const { return m_root; }

    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    Render::GeometryManager<std::string>& geometry_manager() { return m_geometry_manager; }
    TriangleMeshManager<std::string>& triangle_mesh_manager() { return m_trimesh_manager; }

    void render(Render::CommandBuffer& cmd_buffer) const;
    bool pick_at(float mouse_x, float mouse_y, ConstNodePickResults& results) const;
    bool pick_at(float mouse_x, float mouse_y, NodePickResults& results);
private:
    Node m_root;
    Camera m_camera;
    Render::GeometryManager<std::string> m_geometry_manager;
    TriangleMeshManager<std::string> m_trimesh_manager;
};

}

