#pragma once

#include "Buffer.hpp"
#include "VertexAttribDesc.hpp"

namespace Slic3r::App::Render {

class Geometry
{
public:
    Geometry() = default;

    Geometry(Geometry&&) = default;
    Geometry& operator=(Geometry&&) = default;

    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;

    void upload(
        const void* vertex_data,
        size_t vertex_count,
        const VertexAttribsDesc& vertex_format,
        const void* index_data = nullptr,
        size_t index_count = 0,
        IndexType index_format = IndexType::UInt
    );

    void bind(const Shader& shader);
    void draw(GLenum primitive, size_t offset, size_t count) const;
    void unbind();



private:
    std::unique_ptr<VertexBuffer> m_vb;
    std::unique_ptr<IndexBuffer> m_ib;
    GLuint m_vao_id{0};

    VertexAttribsDesc m_vertex_format;
    IndexType m_index_type{IndexType::UInt};
    size_t m_vertex_count{0};
    size_t m_index_count{0};

    // Cached
    GLuint m_shader_id{0};
    std::vector<GLuint> m_shader_attrib_locations;

    bool m_built{false};
};

}