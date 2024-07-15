#include "Geometry.hpp"

namespace Slic3r::App::Render {

size_t size_in_bytes(DataType data_type)
{
    switch (data_type) {
    case DataType::Byte:
        return sizeof(uint8_t);
    case DataType::Float:
        return sizeof(float);
    case DataType::Short:
        return sizeof(uint16_t);
    default:
        // unknown type, please add case for it
        assert(false);
        return 0;
    }
}

size_t VertexAttribDesc::size_in_bytes() const { return Render::size_in_bytes(data_type) * components; }
size_t vertex_attribs_stride(const VertexAttribsDesc& attrs)
{
    size_t ret = 0;
    for (const auto& attr: attrs)
        ret += attr.size_in_bytes();
    return ret;
}

size_t index_type_size(IndexType index)
{
    switch (index) {
    case IndexType::UByte:
        return 1;
    case IndexType::UShort:
        return 2;
    case IndexType::UInt:
        return 4;
    }

    // unsupported index type
    assert(false);
    return 4;
}

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

    const size_t idx = static_cast<size_t>(type);
    assert(0 >= idx && idx < (sizeof(translation_table) / sizeof(GLenum)));

    return translation_table[idx];
}

inline GLenum type(IndexType type)
{
    constexpr static GLenum translation_table[] = {
        // UByte = 0,
        GL_UNSIGNED_BYTE,
        // UShort,
        GL_UNSIGNED_SHORT,
        // UInt
        GL_UNSIGNED_INT,
    };

    const int idx = static_cast<int>(type);
    //assert(0 >= idx && idx < sizeof(translation_table) / sizeof(GLenum));

    return translation_table[idx];
}


const char* shader_input_name(VertexAttribType vat)
{
    switch (vat) {
    case VertexAttribType::Vertex:
        return "v_position";

    case VertexAttribType::Normal:
        return "v_normal";

    case VertexAttribType::TexCoord0:
        return "v_tex_coord";

    case VertexAttribType::Extra:
        return "v_extra";

    }

    // Encountered missing VertexAttribType, if valid, please add it into the switch above
    assert(false);
    return "";
}

} // namespace GL

void Geometry::upload(
    const void* vertex_data,
    size_t vertex_count,
    const VertexAttribsDesc& vertex_format,
    const void* index_data,
    size_t index_count,
    Render::IndexType index_format
)
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
    m_vb->set_data(vertex_data, vertex_attribs_stride(vertex_format) * vertex_count, GL_STATIC_DRAW);
    m_vertex_count = vertex_count;
    m_vertex_format = vertex_format;

    if (index_data && index_count > 0) {
        m_ib = std::make_unique<IndexBuffer>();
        m_ib->set_data(index_data, index_type_size(index_format) * index_count, GL_STATIC_DRAW);
        m_index_count = index_count;
        m_index_type = index_format;
    }

    m_built = true;
}

void Geometry::bind(const Shader& shader)
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

        const size_t stride = vertex_attribs_stride(m_vertex_format);

        for (const auto& vad : m_vertex_format) {
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

void Geometry::draw(GLenum primitive, size_t offset, size_t count) const
{
    assert(m_built);
    if (m_ib)
        glDrawElements(primitive, count, GL::type(m_index_type), reinterpret_cast<const void*>(0 + index_type_size(m_index_type) * offset));
    else
        glDrawArrays(primitive, offset, count);
    glCheck();
}

void Geometry::unbind()
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


} // namespace Slic3r::App::Render
