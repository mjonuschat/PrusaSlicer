//
// Created by Jan Bartipan on 01.03.2024.
//

#pragma once

#include "Node.hpp"
#include "libslic3r/Color.hpp"
#include "slic3r/GUI/GLShader.hpp"

#include <vector>
#include <boost/variant/variant.hpp>
#include "libslic3r/Geometry.hpp"

namespace Slic3r::App::Scene {

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




class Scene {
public:
    Node& root() { return m_root; }
    const Node& root() const { return m_root; }
private:
    Node m_root;

};


}

