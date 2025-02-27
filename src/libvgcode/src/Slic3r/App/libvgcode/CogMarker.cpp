///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#include "CogMarker.hpp"
#include "Utils.hpp"

#include "Slic3r/App/libvgcode/GCodeNodeTag.hpp"

#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Scene/NodeBuilder.hpp>
#include "Slic3r/App/Scene/GeometryDataFactory.hpp"
#include <Slic3r/App/Preview/PreviewSceneLayer.hpp>

#include <assert.h>

namespace Slic3r::App::libvgcode {

void CogMarker::init(Render::Device& device, Scene::NodeBuilder& builder, Scene::GeometryDataFactory& data_factory)
{
    Render::Material material = Render::Material{}
        .set_shader(device.context().shader_manager().get_shader("cog_marker"));

    builder
      .set_debug_name("gcode_cog_marker")
      .set_tag(GCodeNodeTag{ GCodeElementType::CogMarker })
      .set_enabled(false)
      .set_mesh(data_factory.geometry(Scene::GeometryDataId::Sphere), material, int(Preview::PreviewSceneLayer::CogMarker));
}

void CogMarker::update(const Vec3f& position, float mass)
{
    m_total_position = m_total_position + mass * position;
    m_total_mass += mass;
}

void CogMarker::reset()
{
    m_total_position = Vec3f::Zero();
    m_total_mass = 0.0f;
}

Vec3f CogMarker::position() const
{
    assert(m_total_mass > 0.0f);
    return m_total_position / m_total_mass;
}

} // namespace Slic3r::App::libvgcode
