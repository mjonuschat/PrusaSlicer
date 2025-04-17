#include "Slic3r/App/libvgcode/SlaViewer.hpp"

#include <Slic3r/Biz/libpgcode/Utils.hpp>
#include <Slic3r/App/Render/GL/commonGL.hpp>
#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Render/Context.hpp>
#include <Slic3r/App/Render/TextureManager.hpp>
#include <Slic3r/App/Render/TextureBufferManager.hpp>
#include <Slic3r/App/Render/Material.hpp>
#include <Slic3r/App/Scene/NodeBuilder.hpp>
#include <Slic3r/App/Scene/Scene.hpp>
#include "Slic3r/App/Scene/InstancedMeshRenderNodeComponent.hpp"

#include <map>
#include <assert.h>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <algorithm>
#include <cmath>
#include <numeric>

using namespace Slic3r::Biz::libpgcode;
using Slic3r::Domain::Vec3f;

namespace Slic3r::App::libvgcode {

SlaViewer::SlaViewer()
{
}

void SlaViewer::init(Render::Device& device, Scene::Scene& scene, Scene::GeometryDataFactory& data_factory)
{
    if (m_initialized)
        return;

    AbstractViewer::init(device, scene, data_factory);

    Scene::NodeBuilder builder{ *m_scene };
    builder.set_debug_name("sla_main");
//    builder.set_tag(GCodeNodeTag{ GCodeElementType::Undefined });

    builder.child([&](Scene::NodeBuilder& bldr) {
        m_segment_template.init(*m_device, bldr);
    });

    auto main_node = builder.build();
    m_scene->add_child(main_node.release(), &m_scene->root());

    m_initialized = true;
}

void SlaViewer::reset()
{
    AbstractViewer::reset();
    // ToDo: reset other attributes, if needed
}

void SlaViewer::load(const std::vector<float>& layers_zs, const std::vector<float>& layers_times)
{
    if (!m_initialized)
        return;

    if (layers_zs.empty() || layers_times.empty())
        return;

    reset();

    assert(layers_zs.size() == layers_times.size());

    for (size_t i = 0; i < layers_zs.size(); ++i) {
        m_layers.update_as_sla(layers_zs[i], layers_times[i]);
    }

    if (!m_layers.empty())
        m_layers.set_view_range(0, uint32_t(m_layers.count()) - 1);
}

void SlaViewer::render()
{
    if (m_layers.empty())
        return;

    render_segments(m_scene->camera().position().cast<float>());
}

void SlaViewer::set_layers_range(Interval::value_type min, Interval::value_type max)
{
    AbstractViewer::set_layers_range(min, max);

}

void SlaViewer::set_view_visible_range(Interval::value_type min, Interval::value_type max)
{
    AbstractViewer::set_view_visible_range(min, max);

    
}

void SlaViewer::update_view_full_range()
{

}

float SlaViewer::estimated_time() const
{
    return 0.f;
}

float SlaViewer::estimated_time_at(size_t id) const
{
    return 0.f;
}

std::vector<float> SlaViewer::layers_estimated_times() const
{
    return {};
}

void SlaViewer::render_segments(const Vec3f& camera_position)
{
}

} // namespace Slic3r::App::libvgcode
