///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#include "CogMarker.hpp"
#include "Utils.hpp"

#include <assert.h>

namespace Slic3r::Biz::libvgcode {

// Geometry:
// sphere with 'resolution' sides, centered at (0.0, 0.0, 0.0) and radius equal to 'radius'
void CogMarker::init(uint8_t resolution, float radius)
{
//    if (m_vao_id != 0)
//        return;
//
//    // ensure vertices count does not exceed 65536
//    resolution = std::clamp<uint8_t>(resolution, 4, 105);
//
//    uint16_t sector_count = uint16_t(resolution);
//    uint16_t stack_count = uint16_t(resolution);
//
//    float sector_step = 2.0f * float(PI) / float(sector_count);
//    float stack_step = float(PI) / float(stack_count);
//
//    std::vector<float> vertices;
//    uint16_t vertices_count = (stack_count - 1) * sector_count + 2;
//    vertices.reserve(6 * vertices_count);
//
//    m_indices_count = 3 * (2 * (stack_count - 1) * sector_count);
//    std::vector<uint16_t> indices;
//    indices.reserve(m_indices_count);
//
//    // vertices
//    for (uint16_t i = 0; i <= stack_count; ++i) {
//        // from pi/2 to -pi/2
//        float stack_angle = 0.5f * float(PI) - stack_step * float(i);
//        float xy = radius * std::cos(stack_angle);
//        float z = radius * std::sin(stack_angle);
//        if (i == 0 || i == stack_count) {
//            Vec3f pos = { xy, 0.0f, z };
//            Vec3f norm = pos.normalized();
//            add_vertex(pos, norm, vertices);
//        }
//        else {
//            for (uint16_t j = 0; j < sector_count; ++j) {
//                // from 0 to 2pi
//                float sector_angle = sector_step * float(j);
//                Vec3f pos = { xy * std::cos(sector_angle), xy * std::sin(sector_angle), z };
//                Vec3f norm = pos.normalized();
//                add_vertex(pos, norm, vertices);
//            }
//        }
//    }
//
//    // indices
//    for (uint16_t i = 0; i < stack_count; ++i) {
//        // Beginning of current stack.
//        uint16_t k1 = (i == 0) ? 0 : (1 + (i - 1) * sector_count);
//        uint16_t k1_first = k1;
//        // Beginning of next stack.
//        uint16_t k2 = (i == 0) ? 1 : (k1 + sector_count);
//        uint16_t k2_first = k2;
//        for (uint16_t j = 0; j < sector_count; ++j) {
//            // 2 triangles per sector excluding first and last stacks
//            uint16_t k1_next = k1;
//            uint16_t k2_next = k2;
//            if (i != 0) {
//                k1_next = (j + 1 == sector_count) ? k1_first : (k1 + 1);
//                add_triangle(k1, k2, k1_next, indices);
//            }
//            if (i + 1 != stack_count) {
//                k2_next = (j + 1 == sector_count) ? k2_first : (k2 + 1);
//                add_triangle(k1_next, k2, k2_next, indices);
//            }
//            k1 = k1_next;
//            k2 = k2_next;
//        }
//    }
//
//    size_t vertex_stride = 6 * sizeof(float);
//    size_t position_offset = 0;
//    size_t normal_offset = 3 * sizeof(float);
//
//    int curr_vertex_array;
//    glsafe(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &curr_vertex_array));
//    int curr_array_buffer;
//    glsafe(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &curr_array_buffer));
//
//    glsafe(glGenVertexArrays(1, &m_vao_id));
//    glsafe(glBindVertexArray(m_vao_id));
//    glsafe(glGenBuffers(1, &m_vbo_id));
//    glsafe(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));
//    glsafe(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));
//    glsafe(glEnableVertexAttribArray(0));
//    glsafe(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_stride, (const void*)position_offset));
//    glsafe(glEnableVertexAttribArray(1));
//    glsafe(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_stride, (const void*)normal_offset));
//
//    glsafe(glGenBuffers(1, &m_ibo_id));
//    glsafe(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_id));
//    glsafe(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t), indices.data(), GL_STATIC_DRAW));
//
//    glsafe(glBindBuffer(GL_ARRAY_BUFFER, curr_array_buffer));
//    glsafe(glBindVertexArray(curr_vertex_array));
}

void CogMarker::render()
{
//    if (m_vao_id == 0 || m_vbo_id == 0 || m_ibo_id == 0 || m_indices_count == 0)
//        return;
//
//    int curr_vertex_array;
//    glsafe(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &curr_vertex_array));
//    glcheck();
//
//    glsafe(glBindVertexArray(m_vao_id));
//    glsafe(glDrawElements(GL_TRIANGLES, m_indices_count, GL_UNSIGNED_SHORT, (const void*)0));
//    glsafe(glBindVertexArray(curr_vertex_array));
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

} // namespace Slic3r::Biz::libvgcode
