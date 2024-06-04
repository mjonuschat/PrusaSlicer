#pragma once

#include "Buffer.hpp"

namespace Slic3r::App::Render {

/**
 * Vertex Attribute semantic as recognized by Shader
 */
enum class VertexAttribType
{
    Vertex = 0,
    Normal,
    TexCoord0,
    Extra
};


/**
 * Data type of attribute representation in memory
 */
enum class DataType
{
    Float = 0,
    Byte,
    Short
};

/**
 * Description of a single vertex attribute stored in VertexBuffer.
 */
struct VertexAttribDesc
{
    /**
     * Attribute semantic
     */
    VertexAttribType attrib_type;

    /**
     * Attribute data type
     */
    DataType data_type;

    /**
     * Number of components
     */
    uint8_t components;

    /**
     * Offset from start in bytes
     */
    size_t offset;

    /**
     * Stride between vertices in bytes, pass 0 for tightly packed vertex layout.
     */
    //size_t stride;

    /**
     * All components size in bytes for given vertex attribute.
     * @return Vertex attribute size in bytes.
     */
    size_t size_in_bytes() const;
};

using VertexAttribsDesc = std::vector<VertexAttribDesc>;
size_t vertex_attribs_stride(const VertexAttribsDesc& attrs);

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

struct VertexP3T2
{
    Vec3f position;
    Vec2f tex_coord;

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

template <typename I>
struct IndexTypeTraits
{
    static constexpr GLenum type_id = 0;
};

template <>
struct IndexTypeTraits<unsigned int>
{
    static constexpr GLenum type_id = GL_UNSIGNED_INT;
};

template <>
struct IndexTypeTraits<unsigned short>
{
    static constexpr GLenum type_id = GL_UNSIGNED_SHORT;
};

template <>
struct IndexTypeTraits<unsigned char>
{
    static constexpr GLenum type_id = GL_UNSIGNED_BYTE;
};

const char* shader_input_name(VertexAttribType vat);
} // namespace GL


template <typename V, typename I=uint32_t>
class Geometry
{
    static_assert(GL::IndexTypeTraits<I>::type_id != 0, "I parameters must be one of unsigned int, unsigned short, unsigned char");
public:
    Geometry() = default;
    Geometry(Geometry&&) = default;
    Geometry(const Geometry&) = delete;

    using VertexType = V;
    using IndexType = I;

    void upload()
    {
        SPDLOG_TRACE("Buffer::upload() Part 1");
        assert(!m_built);

        auto& ctx = Context::instance();
        // TODO: handle ES with VAO
        bool use_vao = ctx.is_vao_available();

        if (use_vao) {
            glGenVertexArrays(1, &m_vao_id);
            glBindVertexArray(m_vao_id);
            glCheck();
        }
        SPDLOG_TRACE("Buffer::upload() Part 2");

        m_vb = std::make_unique<VertexBuffer>();
        m_vb->set_data(&m_vertices[0], sizeof(Geometry::VertexType) * m_vertices.size(), GL_STATIC_DRAW);
        m_vertices.clear();

        if (!m_indices.empty()) {
            m_ib = std::make_unique<IndexBuffer>();
            m_ib->set_data(&m_indices[0], sizeof(IndexType) * m_indices.size(), GL_STATIC_DRAW);
            m_indices.clear();
        }
        m_built = true;
    }

    void bind(const Shader& shader)
    {
        assert(m_built);

        auto& ctx = Context::instance();
        // TODO: handle ES with VAO
        bool use_vao = ctx.is_vao_available();

        if (use_vao)
            glBindVertexArray(m_vao_id);

        bool needs_new_binding = !use_vao || m_shader_id != shader.m_id;

        if (needs_new_binding) {
            m_vb->bind();

            const auto& format = VertexType::format();
            const size_t stride = vertex_attribs_stride(format);

            for (const auto& vad : format) {
                int loc = shader.get_attrib_location(GL::shader_input_name(vad.attrib_type));
                if (loc < 0)
                    continue;
                glEnableVertexAttribArray(loc);
                glVertexAttribPointer(
                    loc, vad.components, GL::type(vad.data_type), GL_FALSE, stride,
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
        if (m_ib)
            glDrawElements(primitive, count, GL::IndexTypeTraits<I>::type_id, reinterpret_cast<const void*>(0 + sizeof(I) * offset));
        else
            glDrawArrays(primitive, offset, count);
        glCheck();
    }

    void unbind()
    {
        auto& ctx = Context::instance();
        bool use_vao = ctx.is_vao_available();
        if (use_vao) {
            glBindVertexArray(0);
        } else {
            // TODO: unbind enabled attrs
        }
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

    size_t m_vertex_count{0};
    size_t m_index_count{0};

    // Cached
    GLuint m_shader_id{0};
    std::vector<GLuint> m_shader_attrib_locations;

    bool m_built{false};
};

}