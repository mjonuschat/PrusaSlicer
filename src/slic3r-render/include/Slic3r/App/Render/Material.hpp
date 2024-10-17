#pragma once

#include "Types.hpp"
#include "UniformValue.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace Slic3r::App::Render {

struct Material
{
    using Uniforms = std::unordered_map<std::string, UniformValue>;

    std::string shader_name;
    Uniforms uniforms;
    bool transparent;
};

struct MaterialOverride
{
    std::optional<std::string> shader_name;
    Material::Uniforms uniforms;
    std::optional<bool> transparent;
};

}