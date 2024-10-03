#pragma once

#include <vector>
#include <boost/variant/variant.hpp>

#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Scene/GeometryManager.hpp"
#include "Slic3r/App/Render/Shader.hpp"

#include "libslic3r/Color.hpp"
#include "libslic3r/Geometry.hpp"


namespace Slic3r::App::Scene {

class Scene {
public:
    Node& root() { return m_root; }
    const Node& root() const { return m_root; }

    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    GeometryManager& geometry_manager() { return m_geometry_manager; }

    void render(Render::Device& device) const;
private:
    Node m_root;
    Camera m_camera;
    GeometryManager m_geometry_manager;
};


}

