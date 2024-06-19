#include "GeometryBuilder.hpp"

namespace Slic3r::App::Render {

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



}
