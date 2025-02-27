///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#include "OptionTemplate.hpp"
#include "Utils.hpp"

#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Render/Context.hpp>
#include "Slic3r/App/libvgcode/GCodeNodeTag.hpp"
#include <Slic3r/App/Preview/PreviewSceneLayer.hpp>
#include <Slic3r/App/Scene/NodeBuilder.hpp>
#include "Slic3r/App/Scene/GeometryDataFactory.hpp"
#include <Slic3r/App/Scene/Scene.hpp>

#include <cmath>

namespace Slic3r::App::libvgcode {

// Geometry:
// diamond with 'resolution' sides, centered at (0.0, 0.0, 0.0)
// height and width of the diamond are equal to 1.0
void OptionTemplate::init(Render::Device& device, Scene::NodeBuilder& builder, Scene::GeometryDataFactory& data_factory)
{
    Render::Material material = Render::Material{}
        .set_shader(device.context().shader_manager().get_shader("options"));

    builder
        .set_debug_name("gcode_options")
        .set_tag(GCodeNodeTag{ GCodeElementType::Options })
        .set_mesh_instanced(data_factory.geometry(Scene::GeometryDataId::CandyButton), material, 0,
            Render::PrimitiveType::Triangles, int(Preview::PreviewSceneLayer::Options));
}

} // namespace Slic3r::App::libvgcode
