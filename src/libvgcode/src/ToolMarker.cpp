///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#include "ToolMarker.hpp"
#include "Utils.hpp"

#include <cmath>

namespace Slic3r::Biz::libvgcode {

// Geometry:
// Arrow with cylindrical stem and conical tip, with the given dimensions and resolution
// The origin of the arrow is at the tip of the conical section
// The axis of symmetry is along the Z axis
// The arrow is pointing downward
void ToolMarker::init(uint16_t resolution, float tip_radius, float tip_height, float stem_radius, float stem_height)
{
//    if (m_vao_id != 0)
//        return;
//
//    // ensure vertices count does not exceed 65536
//    resolution = std::clamp<uint16_t>(resolution, 4, 10922);
//
//    std::vector<float> vertices;
//    const uint16_t vertices_count = 6 * resolution + 2;
//    vertices.reserve(6 * vertices_count);
//
//    m_indices_count = 6 * resolution * 3;
//    std::vector<uint16_t> indices;
//    indices.reserve(m_indices_count);
//
//    const float angle_step = 2.0f * float(PI) / float(resolution);
//    std::vector<float> cosines(resolution);
//    std::vector<float> sines(resolution);
//
//    for (uint16_t i = 0; i < resolution; ++i) {
//        const float angle = angle_step * float(i);
//        cosines[i] = std::cos(angle);
//        sines[i] = -std::sin(angle);
//    }
//
//    const float total_height = tip_height + stem_height;
//
//    // tip vertices
//    add_vertex(VEC3_ZERO, -VEC3_UNIT_Z, vertices);
//    for (uint16_t i = 0; i < resolution; ++i) {
//        add_vertex({ tip_radius * sines[i], tip_radius * cosines[i], tip_height }, { sines[i], cosines[i], 0.0f }, vertices);
//    }
//
//    // tip triangles
//    for (uint16_t i = 0; i < resolution; ++i) {
//        const uint16_t v3 = (i < resolution - 1) ? i + 2 : 1;
//        add_triangle(0, i + 1, v3, indices);
//    }
//
//    // tip cap outer perimeter vertices
//    for (uint16_t i = 0; i < resolution; ++i) {
//      add_vertex({ tip_radius * sines[i], tip_radius * cosines[i], tip_height }, VEC3_UNIT_Z, vertices);
//    }
//
//    // tip cap inner perimeter vertices
//    for (uint16_t i = 0; i < resolution; ++i) {
//        add_vertex({ stem_radius * sines[i], stem_radius * cosines[i], tip_height }, VEC3_UNIT_Z, vertices);
//    }
//
//    // tip cap triangles
//    for (uint16_t i = 0; i < resolution; ++i) {
//        const uint16_t v2 = (i < resolution - 1) ? i + resolution + 2 : resolution + 1;
//        const uint16_t v3 = (i < resolution - 1) ? i + 2 * resolution + 2 : 2 * resolution + 1;
//        add_triangle(i + resolution + 1, v3, v2, indices);
//        add_triangle(i + resolution + 1, i + 2 * resolution + 1, v3, indices);
//    }
//
//    // stem bottom vertices
//    for (uint16_t i = 0; i < resolution; ++i) {
//        add_vertex({ stem_radius * sines[i], stem_radius * cosines[i], tip_height }, { sines[i], cosines[i], 0.0f }, vertices);
//    }
//
//    // stem top vertices
//    for (uint16_t i = 0; i < resolution; ++i) {
//        add_vertex({ stem_radius * sines[i], stem_radius * cosines[i], total_height }, { sines[i], cosines[i], 0.0f }, vertices);
//    }
//
//    // stem triangles
//    for (uint16_t i = 0; i < resolution; ++i) {
//        const uint16_t v2 = (i < resolution - 1) ? i + 3 * resolution + 2 : 3 * resolution + 1;
//        const uint16_t v3 = (i < resolution - 1) ? i + 4 * resolution + 2 : 4 * resolution + 1;
//        add_triangle(i + 3 * resolution + 1, v3, v2, indices);
//        add_triangle(i + 3 * resolution + 1, i + 4 * resolution + 1, v3, indices);
//    }
//
//    // stem cap vertices
//    add_vertex({ 0.0f, 0.0f, total_height }, VEC3_UNIT_Z, vertices);
//    for (uint16_t i = 0; i < resolution; ++i) {
//        add_vertex({ stem_radius * sines[i], stem_radius * cosines[i], total_height }, VEC3_UNIT_Z, vertices);
//    }
//
//    // stem cap triangles
//    for (uint16_t i = 0; i < resolution; ++i) {
//        const uint16_t v3 = (i < resolution - 1) ? i + 5 * resolution + 3 : 5 * resolution + 2;
//        add_triangle(5 * resolution + 1, v3, i + 5 * resolution + 2, indices);
//    }
//
//    m_bounding_box[0] = { FLT_MAX, FLT_MAX, FLT_MAX };
//    m_bounding_box[1] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
//    for (std::size_t i = 0; i < vertices.size(); ++i) {
//        const std::size_t id = i % 6;
//        if (id < 3) {
//            m_bounding_box[0][id] = std::min(m_bounding_box[0][id], vertices[i]);
//            m_bounding_box[1][id] = std::max(m_bounding_box[1][id], vertices[i]);
//        }
//    }
//
//    m_size_in_bytes_gpu += vertices.size() * sizeof(float);
//    m_size_in_bytes_gpu += indices.size() * sizeof(uint16_t);
//
//    const std::size_t vertex_stride = 6 * sizeof(float);
//    const std::size_t position_offset = 0;
//    const std::size_t normal_offset = 3 * sizeof(float);
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

void ToolMarker::render()
{
//    if (m_vao_id == 0 || m_vbo_id == 0 || m_ibo_id == 0)
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

} // namespace Slic3r::Biz::libvgcode
