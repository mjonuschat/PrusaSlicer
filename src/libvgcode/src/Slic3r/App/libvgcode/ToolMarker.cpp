///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#include "ToolMarker.hpp"
#include "Utils.hpp"
#include "Slic3r/App/libvgcode/GCodeNodeTag.hpp"

#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Scene/NodeBuilder.hpp>
#include "Slic3r/App/Scene/GeometryDataFactory.hpp"
#include <Slic3r/App/Preview/PreviewSceneLayer.hpp>

#include <cmath>

namespace Slic3r::App::libvgcode {

void ToolMarker::init(Render::Device& device, Scene::NodeBuilder& builder, Scene::GeometryDataFactory& data_factory)
{
    Render::Material material = Render::Material{}
        .set_shader(device.context().shader_manager().get_shader("tool_marker"));

    builder
        .set_debug_name("gcode_tool_marker")
        .set_tag(GCodeNodeTag{ GCodeElementType::ToolMarker })
        .set_mesh(data_factory.geometry(Scene::GeometryDataId::ToolMarker), material, int(Preview::PreviewSceneLayer::ToolMarker));
}

} // namespace Slic3r::App::libvgcode
