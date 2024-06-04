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

const VertexAttribsDesc& VertexP3::format()
{
    static const VertexAttribsDesc desc = {
        {VertexAttribType::Vertex, DataType::Float, 3, 0}
    };
    return desc;
}

const VertexAttribsDesc& VertexP3N3::format()
{
    static const VertexAttribsDesc desc = {
        {VertexAttribType::Vertex, DataType::Float, 3, offsetof(VertexP3N3, position)},
        {VertexAttribType::Normal, DataType::Float, 3, offsetof(VertexP3N3, normal)}
    };
    return desc;
}

const VertexAttribsDesc& VertexP3T2::format()
{
    static const VertexAttribsDesc desc =  {
        {VertexAttribType::Vertex, DataType::Float, 3, offsetof(VertexP3T2, position)},
        {VertexAttribType::TexCoord0, DataType::Float, 2, offsetof(VertexP3T2, tex_coord)}
    };
    return desc;
}

const VertexAttribsDesc& VertexP3N3T2::format()
{
    static const VertexAttribsDesc desc =  {
        {VertexAttribType::Vertex, DataType::Float, 3, offsetof(VertexP3N3T2, position)},
        {VertexAttribType::Normal, DataType::Float, 3, offsetof(VertexP3N3T2, normal)},
        {VertexAttribType::TexCoord0, DataType::Float, 2, offsetof(VertexP3N3T2, tex_coord)}
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
        return "v_tex_coord";

    case VertexAttribType::Extra:
        return "v_extra";

    }

    // Encountered missing VertexAttribType, if valid, please add it into the switch above
    assert(false);
    return "";
}

} // namespace GL

} // namespace Slic3r::App::Render
