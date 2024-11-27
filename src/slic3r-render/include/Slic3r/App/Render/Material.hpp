#pragma once

#include <string>
#include <unordered_map>

#include "Slic3r/App/Render/Shader.hpp"
#include "Slic3r/App/Render/UniformValue.hpp"
#include "Slic3r/App/Render/Texture.hpp"

namespace Slic3r::App::Render {

using MaterialTextures = std::unordered_map<size_t, const Texture*>;
using MaterialUniforms = std::unordered_map<std::string, UniformValue>;

class Material
{
public:
    Material() = default;
    Material(const Material&) = default;
    Material(Material&&) = default;

    Material& operator=(const Material&) = default;
    Material& operator=(Material&&) = default;

    explicit Material(const Shader* shader) : m_shader(shader) {}

    const Shader* shader() const { return m_shader; }
    Material& set_shader(const Shader* shader) { m_shader = shader; return *this; }

    const MaterialTextures& textures() const { return m_textures; }
    MaterialTextures& textures() { return m_textures; }

    const MaterialUniforms& uniforms() const { return m_uniforms; }
    MaterialUniforms& uniforms() { return m_uniforms; }

    Material& set_uniform(const std::string& name, const UniformValue& value)
    {
        m_uniforms[name] = value;
        return *this;
    }

    Material& set_texture(size_t slot, const Texture* texture)
    {
        m_textures[slot] = texture;
        return *this;
    }

    bool transparent() const { return m_transparent.has_value() ? *m_transparent : false; }
    Material& set_transparent(bool transparent)
    {
        m_transparent = transparent;
        return *this;
    }

    void update(const Material& override);
private:
    const Shader* m_shader{nullptr};
    MaterialUniforms m_uniforms;
    MaterialTextures m_textures;
    std::optional<bool> m_transparent{};
};

}

