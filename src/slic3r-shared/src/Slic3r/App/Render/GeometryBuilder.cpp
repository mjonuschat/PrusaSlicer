#include "Slic3r/App/Render/GeometryBuilder.hpp"

#include <libslic3r/TriangleMesh.hpp>

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

std::unique_ptr<Geometry> geometry_from_triangle_mesh(Device& device, const TriangleMesh& triangle_mesh)
{ return geometry_from_triangle_mesh(device, triangle_mesh.its); }

std::unique_ptr<Geometry> geometry_from_triangle_mesh(Device& device, const indexed_triangle_set& triangle_mesh)
{
    const size_t num_verts = triangle_mesh.indices.size() * 3;
    GeometryBuilder<VertexP3N3> builder;
    builder.reserve(num_verts, 0);

    for (const auto& tri : triangle_mesh.indices) {
        const auto n = its_face_normal(triangle_mesh, tri);
        builder
            .add_vertex({triangle_mesh.vertices[tri[0]], n})
            .add_vertex({triangle_mesh.vertices[tri[1]], n})
            .add_vertex({triangle_mesh.vertices[tri[2]], n});
    }
    return builder.build(device);
}

}
