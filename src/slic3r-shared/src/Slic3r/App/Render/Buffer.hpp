#pragma once

#include <memory>

#include <spdlog/spdlog.h>

#include "commonGL.hpp"
#include "Shader.hpp"
#include "Context.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::App::Render {

class Buffer {
public:
    explicit Buffer(GLenum target): m_target(target)
    {
        glGenBuffers(1, &m_id);
        glCheck();
    }

    virtual ~Buffer();

    inline void bind() const
    {
        glBindBuffer(m_target, m_id);
        glCheck();
    }

    inline void set_data(const void* data, GLsizeiptr size, GLenum usage)
    {
        bind();
        glBufferData(m_target, size, data, usage);
        glCheck();
    }

private:
    GLenum m_target;
    GLuint m_id {0};
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer() : Buffer(GL_ARRAY_BUFFER) {}

    inline void bind_vertex_attrib(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, const void* pointer)
    {
        glVertexAttribPointer(index, size, type, normalized ? GL_TRUE : GL_FALSE, stride, pointer);
        glCheck();
    }
};

class IndexBuffer : public Buffer
{
public:
    IndexBuffer() : Buffer(GL_ELEMENT_ARRAY_BUFFER) {}
};

enum class VertexAttribType
{
    Vertex = 0,
    Normal,
    TexCoord0,
    Extra
};

enum class DataType
{
    Float = 0,
    Byte,
    Short
};

struct VertexAttribDesc
{
    VertexAttribType attrib_type;
    DataType data_type;
    uint8_t components;
    size_t offset;
    size_t stride;

    size_t size() const;
};

using VertexAttribsDesc = std::vector<VertexAttribDesc>;

struct VertexP3
{
    Vec3f position;

    static const VertexAttribsDesc& format();
};

struct VertexP3N3
{
    Vec3f position;
    Vec3f normal;

    static const VertexAttribsDesc& format();
};

struct VertexP3N3T2
{
    Vec3f position;
    Vec3f normal;
    Vec2f tex_coord;

    static const VertexAttribsDesc& format();
};

static_assert(sizeof(VertexP3) == 3 * 4);
static_assert(sizeof(VertexP3N3) == (3 + 3) * 4);
static_assert(sizeof(VertexP3N3T2) == (3 + 3 + 2) * 4);

namespace GL {
inline GLenum type(DataType type)
{
    constexpr static GLenum translation_table[] = {
        // Float = 0,
        GL_FLOAT,
        // Byte,
        GL_BYTE,
        // Short
        GL_SHORT
    };

    const int idx = static_cast<int>(type);
    assert(0 >= idx && idx < (sizeof(translation_table) / sizeof(GLenum)));

    return translation_table[idx];
}

const char* shader_input_name(VertexAttribType vat);
}


template <typename V, typename I=uint32_t>
class Geometry
{
public:
    Geometry() = default;
    Geometry(Geometry&&) = default;
    Geometry(const Geometry&) = delete;

    using VertexType = V;
    using IndexType = I;

    void upload()
    {
        SPDLOG_INFO("Buffer::upload() Part 1");
        assert(!m_built);
        glGenVertexArrays(1, &m_vao_id);
        glBindVertexArray(m_vao_id);
        glCheck();
        SPDLOG_INFO("Buffer::upload() Part 2");

        m_vb = std::make_unique<VertexBuffer>();
        m_vb->set_data(&m_vertices[0], sizeof(Geometry::VertexType) * m_vertices.size(), GL_STATIC_DRAW);
        m_vertices.clear();

        if (!m_indices.empty()) {
            m_ib = std::make_unique<IndexBuffer>();
            m_ib->set_data(&m_indices[0], sizeof(IndexType) * m_indices.size(), GL_STATIC_DRAW);
            m_indices.clear();
        }

//        m_vb->bind();
//        const auto& format = VertexType::format();
//        for (const auto& vad : format) {
//            int index = static_cast<int>(vad.attrib_type);
//            glEnableVertexAttribArray(index);
//            glVertexAttribPointer(index, vad.components, GL::type(vad.data_type), GL_FALSE, 0, reinterpret_cast<void *>(vad.offset));
//            glCheck();
//        }
//        if (m_ib) {
//            m_ib->bind();
//        }
        m_built = true;
    }

    void bind(const Shader& shader)
    {
        assert(m_built);

        auto& ctx = Context::instance();
        // TODO: handle ES with VAO
        bool use_vao = !ctx.is_es();

        glBindVertexArray(m_vao_id);

        bool needs_new_binding = m_shader_id != shader.m_id;

        if (needs_new_binding) {
            m_vb->bind();

            const auto& format = VertexType::format();
            for (const auto& vad : format) {
                int loc = shader.get_attrib_location(GL::shader_input_name(vad.attrib_type));
                if (loc < 0)
                    continue;
                glEnableVertexAttribArray(loc);
                glVertexAttribPointer(
                    loc, vad.components, GL::type(vad.data_type), GL_FALSE, 0,
                    reinterpret_cast<void*>(vad.offset)
                );
                glCheck();
            }

            if (m_ib) {
                m_ib->bind();
                glCheck();
            }

            m_shader_id = shader.m_id;
        }
    }

    void draw(GLenum primitive, size_t offset, size_t count) const
    {
        assert(m_built);
        glDrawArrays(primitive, offset, count);
        glCheck();
    }

    void unbind()
    {
        glBindVertexArray(0);
        glCheck();
    }

    Geometry& add_vertex(const V& v)
    {
        m_vertices.push_back(v);
        return *this;
    }

    Geometry& add_index(IndexType i)
    {
        m_indices.push_back(i);
        return *this;
    }

    Geometry& add_triangle_indices(IndexType i0, IndexType i1, IndexType i2)
    {
        m_indices.push_back(i0);
        m_indices.push_back(i1);
        m_indices.push_back(i2);
        return *this;
    }


private:
    std::vector<V> m_vertices;
    std::vector<I> m_indices;
    std::unique_ptr<VertexBuffer> m_vb;
    std::unique_ptr<IndexBuffer> m_ib;
    GLuint m_vao_id{0};

    // Cached
    GLuint m_shader_id{0};
    std::vector<GLuint> m_shader_attrib_locations;

    bool m_built{false};
};


} // namespace Slic3r::App::Render
