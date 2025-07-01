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

float AbstractViewer::encoded_color(const Domain::ColorRGB& color) {
    int r = int(color.r_uchar());
    int g = int(color.g_uchar());
    int b = int(color.b_uchar());
    int i_color = r << 16 | g << 8 | b;
    return float(i_color);
}

void AbstractViewer::set_lights(const Scene::Lighting& lights)
{
    m_lights.ambient_intensity = lights.ambient_intensity;
    m_lights.lights.clear();
    size_t num_lights = std::min(lights.lights.size(), Scene::MAX_NUM_LIGHTS);
    m_lights.lights.reserve(num_lights);
    for (size_t i = 0; i < num_lights; ++i) {
        Scene::Light light = lights.lights[i];
        light.direction = light.direction.normalized();
        light.ambient = std::max(light.ambient, 0.0f);
        light.diffuse = std::max(light.diffuse, 0.0f);
        light.specular = std::max(light.specular, 0.0f);
        // avoid shininess == 0.0, see: https://registry.khronos.org/OpenGL-Refpages/gl4/html/pow.xhtml
        light.shininess = std::max(light.shininess, 0.001f);

        m_lights.lights.emplace_back(light);
    }
}

} // namespace Slic3r::App::libvgcode
