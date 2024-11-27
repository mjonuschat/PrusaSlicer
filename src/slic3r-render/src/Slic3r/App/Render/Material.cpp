#include "Slic3r/App/Render/Material.hpp"

namespace Slic3r::App::Render {

template<typename K, typename V>
void update_map(std::unordered_map<K, V>& dest, const std::unordered_map<K, V>& override)
{
    for (const auto& [k, v] : override)
        dest[k] = v;
}

void Material::update(const Material& override)
{
    auto* shader = override.shader();
    if (shader)
        m_shader = shader;
    update_map(m_uniforms, override.uniforms());
    update_map(m_textures, override.textures());
    if (override.m_transparent.has_value())
        m_transparent = override.transparent();
}

} // namespace Slic3r::App::Render
