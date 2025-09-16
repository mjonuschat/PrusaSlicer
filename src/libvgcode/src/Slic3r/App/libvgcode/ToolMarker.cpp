///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/libvgcode/ToolMarker.hpp"
#include "Slic3r/App/libvgcode/Utils.hpp"
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
        .set_shader(device.context().shader_manager().shader("tool_marker"));

    const auto* geom = data_factory.geometry(Scene::GeometryDataId::ToolMarker);
    const auto& trimesh = data_factory.triangle_mesh(Scene::GeometryDataId::ToolMarker);

    builder
        .set_debug_name("gcode_tool_marker")
        .set_tag(GCodeNodeTag{ GCodeElementType::ToolMarker })
        .set_mesh(geom, material, int(Preview::PreviewSceneLayer::ToolMarker))
        // add collision geometry to let the tool marker be taken in account by camera frustum tighting,
        // see: CameraFrustumUpdater::update_camera_frustum
        .set_aabb(trimesh->aabb_mesh());
}

} // namespace Slic3r::App::libvgcode
