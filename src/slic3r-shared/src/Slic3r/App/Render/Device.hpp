#pragma once

#include <vector>
#include <cstddef>

namespace Slic3r::App::Render {

using ResourceId = unsigned int;
using ResourceIds = std::vector<ResourceId>;

class Context;
class Texture;
class VertexBuffer;
class IndexBuffer;
class Shader;

class Device
{
    friend class Context;
    explicit Device(Context& context): m_context(context) {}
public:

    std::unique_ptr<Texture> create_texture();
    std::unique_ptr<VertexBuffer> create_vertex_buffer();
    std::unique_ptr<IndexBuffer> create_index_buffer();
private:
    Context& m_context;
    ResourceId m_bound_vertex_buffer{0};
    ResourceId m_bound_index_buffer{0};
    ResourceIds m_bound_textures{0};
    size_t m_active_texture_unit{0};
};

}
