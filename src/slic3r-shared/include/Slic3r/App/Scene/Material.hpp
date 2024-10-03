#pragma once

#include <string>
#include <unordered_map>

#include "Slic3r/App/Render/Shader.hpp"
#include "Slic3r/App/Render/UniformValue.hpp"
#include "Slic3r/App/Render/Texture.hpp"

namespace Slic3r::App::Scene {

using MaterialTextures = std::unordered_map<size_t, const Render::Texture*>;
using MaterialUniforms = std::unordered_map<std::string, Render::UniformValue>;

class Material
{
public:
    Material() = default;
    Material(const Material&) = default;
    Material(Material&&) = default;

    Material& operator=(const Material&) = default;
    Material& operator=(Material&&) = default;

    explicit Material(const Render::Shader* shader) : m_shader(shader) {}

    const Render::Shader* shader() const { return m_shader; }
    void set_shader(const Render::Shader* shader) { m_shader = shader; }

    const MaterialTextures& textures() const { return m_textures; }
    MaterialTextures& textures() { return m_textures; }

    const MaterialUniforms& uniforms() const { return m_uniforms; }
    MaterialUniforms& uniforms() { return m_uniforms; }

    Material& set_uniform(const std::string& name, const Render::UniformValue& value)
    {
        m_uniforms[name] = value;
        return *this;
    }

    Material& set_texture(size_t slot, const Render::Texture* texture)
    {
        m_textures[slot] = texture;
        return *this;
    }

    void update(const Material& override);
private:
    const Render::Shader* m_shader{nullptr};
    MaterialUniforms m_uniforms;
    MaterialTextures m_textures;
};

}
