#include "Slic3r/App/libvgcode/AbstractViewer.hpp"
#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Scene/Scene.hpp>

namespace Slic3r::App::libvgcode {

void AbstractViewer::init(Render::Device& device, Scene::Scene& scene, Scene::GeometryDataFactory& data_factory)
{
    m_device = &device;
    m_scene = &scene;
}

void AbstractViewer::reset()
{
    m_layers.reset();
    m_view_range.reset();
}


void AbstractViewer::set_layers_range(Interval::value_type min, Interval::value_type max)
{
    min = std::clamp<Interval::value_type>(min, 0, m_layers.count() - 1);
    max = std::clamp<Interval::value_type>(max, 0, m_layers.count() - 1);
    m_layers.set_view_range(min, max);
    // force immediate update of the full range
    update_view_full_range();
    m_view_range.set_visible(m_view_range.enabled());
}

void AbstractViewer::set_view_visible_range(Interval::value_type min, Interval::value_type max)
{
    // force update of the full range, to avoid clamping the visible range with full old values
    // when calling m_view_range.set_visible()
    update_view_full_range();
    m_view_range.set_visible(min, max);
}

void AbstractViewer::set_lights(const Lights& lights)
{
    m_lights.clear();
    size_t num_lights = std::min(lights.size(), MAX_NUM_LIGHTS);
    m_lights.reserve(num_lights);
    for (size_t i = 0; i < num_lights; ++i) {
        Light light = lights[i];
        light.direction = light.direction.normalized();
        light.ambient = std::max(light.ambient, 0.0f);
        light.diffuse = std::max(light.diffuse, 0.0f);
        light.specular = std::max(light.specular, 0.0f);
        light.shininess = std::max(light.shininess, 0.0f);

        // specular and shininess cannot be both zero, see: https://registry.khronos.org/OpenGL-Refpages/gl4/html/pow.xhtml
        if (light.specular == 0.0f && light.shininess == 0.0f)
            light.shininess = 0.001f;

        m_lights.emplace_back(light);
    }
}

static const Lights DEFAULT_LIGHTS = {
    { LightReferenceSystem::Eye, { -0.4574957f, 0.4574957f, 0.7624929f }, 0.45f, 0.48f, 0.075f, 20.0f },
    { LightReferenceSystem::Eye, { 0.70014f, 0.140028f, 0.70014f }, 0.0f, 0.18f, 0.0f, 0.0f }
};

const Lights& AbstractViewer::default_lights() const
{
    return DEFAULT_LIGHTS;
}


} // namespace Slic3r::App::libvgcode
