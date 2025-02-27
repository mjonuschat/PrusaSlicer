///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#include "SegmentTemplate.hpp"

#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Render/Context.hpp>
#include "Slic3r/App/libvgcode/GCodeNodeTag.hpp"
#include <Slic3r/App/Preview/PreviewSceneLayer.hpp>
#include <Slic3r/App/Scene/NodeBuilder.hpp>
#include <Slic3r/App/Scene/Scene.hpp>

#include <cstdint>
#include <array>

namespace Slic3r::App::libvgcode {

//|     /1-------6\     |
//|    / |       | \    |
//|   2--0-------5--7   |
//|    \ |       | /    |
//|      3-------4      | 
static constexpr const std::array<uint8_t, 24> VERTEX_DATA = {
    0, 1, 2, // front spike
    0, 2, 3, // front spike
    0, 3, 4, // right/bottom body 
    0, 4, 5, // right/bottom body 
    0, 5, 6, // left/top body 
    0, 6, 1, // left/top body 
    5, 4, 7, // back spike
    5, 7, 6, // back spike
};

void SegmentTemplate::init(Render::Device& device, Scene::NodeBuilder& builder)
{
    Render::VertexAttribDesc v_attr;
    v_attr.attrib_type = Render::VertexAttribType::Vertex;
    v_attr.components = 1;
    v_attr.data_type = Render::DataType::UByte;
    v_attr.normalize = false;
    v_attr.offset = 0;

    m_geometry = std::make_unique<Render::Geometry>(device);
    m_geometry->upload(VERTEX_DATA.data(), VERTEX_DATA.size(), { v_attr });

    Render::Material material = Render::Material{}
        .set_shader(device.context().shader_manager().get_shader("segments"));

    Render::DrawCommands draw_commands;
    draw_commands.push_back({ Render::PrimitiveType::Triangles, 0, m_geometry->vertex_count(), material});
    m_geometry->draw_commands() = draw_commands;

    builder
        .set_debug_name("gcode_toolpaths")
        .set_tag(GCodeNodeTag{ GCodeElementType::Toolpaths })
        .set_mesh_instanced(m_geometry.get(), material, 0, Render::PrimitiveType::Triangles,
            int(Preview::PreviewSceneLayer::Toolpaths));
}

} // namespace Slic3r::App::libvgcode
