#pragma once

#include "Slic3r/App/Render/Types.hpp"

#include "libslic3r/Point.hpp"
#include "libslic3r/Color.hpp"

#include <array>
#include <string_view>
#include <variant>

namespace Slic3r::App::Render {

class Shader;

using UniformValue = std::variant<
    float
    , int
    , bool
    , Vec2f
    , Vec3f
    , Matrix3f
    , Matrix4f
    , ColorRGB
    , ColorRGBA

>;

void set_uniform(Shader& shader, const char* param_name, const UniformValue& value);

}