#include <cstddef>  // contains offsetof

#include "Buffer.hpp"

namespace Slic3r::App::Render {

Buffer::~Buffer()
{
    if (m_id)
        glDeleteBuffers(1, &m_id);
}

size_t size(DataType data_type)
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

#define OFFSET_OF(T, member) size_t(&reinterpret_cast<T*>(0)->member)

size_t VertexAttribDesc::size() const { return Render::size(data_type) * components; }
const VertexAttribsDesc& VertexP3::format()
{
    static const VertexAttribsDesc desc = {
        {VertexAttribType::Vertex, DataType::Float, 3, 0, 0}
    };
    return desc;
}

const VertexAttribsDesc& VertexP3N3::format()
{
    static const VertexAttribsDesc desc = {
        {VertexAttribType::Vertex, DataType::Float, 3, offsetof(VertexP3N3, position), 0},
        {VertexAttribType::Normal, DataType::Float, 3, offsetof(VertexP3N3, normal), 0}
    };
    return desc;
}

const VertexAttribsDesc& VertexP3N3T2::format()
{
    static const VertexAttribsDesc desc =  {
        {VertexAttribType::Vertex, DataType::Float, 3, offsetof(VertexP3N3T2, position), 0},
        {VertexAttribType::Normal, DataType::Float, 3, offsetof(VertexP3N3T2, normal), 0},
        {VertexAttribType::TexCoord0, DataType::Float, 2, offsetof(VertexP3N3T2, tex_coord), 0}
    };
    return desc;
}

namespace GL {

const char* shader_input_name(VertexAttribType vat)
{
    switch (vat) {
    case VertexAttribType::Vertex:
        return "v_position";

    case VertexAttribType::Normal:
        return "v_normal";

    case VertexAttribType::TexCoord0:
        return "v_texcoord";

    case VertexAttribType::Extra:
        return "v_extra";

    }

    // Encountered missing VertexAttribType, if valid, please add it into the switch above
    assert(false);
    return "";
}

}

} // namespace Slic3r::App::Render
