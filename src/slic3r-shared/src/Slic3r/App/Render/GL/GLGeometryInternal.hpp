#pragma once

#include "Slic3r/App/Render/Geometry.hpp"
#include "commonGL.hpp"

namespace Slic3r::App::Render::GL {

struct GLGeometryInternal : public Geometry::Internal {
    GLuint m_vao_id{0};
    // Cached
    GLuint m_shader_id{0};
    std::vector<GLuint> m_shader_attrib_locations;
};

}
