#include "Slic3r/App/Scene/Material.hpp"

namespace Slic3r::App::Scene {

template <typename K, typename V>
void update_map(std::unordered_map<K, V>& dest, const std::unordered_map<K, V>& override)
{
    for (const auto& [k, v] : override)
        dest[k] = v;
}

void Material::update(const Material& override)
{
    if (override.shader())
        m_shader = override.shader();
    update_map(m_uniforms, override.uniforms());
    update_map(m_textures, override.textures());
}

}