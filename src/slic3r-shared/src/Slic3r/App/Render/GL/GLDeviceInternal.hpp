#pragma once

#include <vector>
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/Types.hpp"

namespace Slic3r::App::Render {
class Geometry;
class Shader;
class Context;
}

namespace Slic3r::App::Render::GL {

using ResourceId = unsigned int;
using ResourceIds = std::vector<ResourceId>;


class GLDeviceInternal : public Device::Internal
{
public:
    explicit GLDeviceInternal(Context& context);

    void load_state();

    void bind_shader(Shader& s);
    void bind_buffer(BufferTarget target, ResourceId buffer);
    void bind_geometry(Geometry& g, Shader& s);
    void unbind_geometry();
    void bind_texture(uint8_t unit, Texture& t);
    void unbind_texture(uint8_t unit, Texture& t);

    void draw(PrimitiveType primitive, size_t offset, size_t count);
private:
    friend class ::Slic3r::App::Render::Geometry;
    void activate_texture_unit(uint8_t unit);
    void bind_vertex_buffer(ResourceId vb);
    void bind_index_buffer(ResourceId vb);
    void bind_vao(ResourceId vao);

    void print_buffer_info(const char* action = nullptr);
private:
    Context& m_context;
    // only active bound VB (not taking bound VAO into account)
    ResourceId m_bound_vertex_buffer{0};
    // only active bound IB (not taking bound VAO into account)
    ResourceId m_bound_index_buffer{0};
    IndexType m_bound_index_type{IndexType::UByte};
    ResourceId m_bound_vao{0};
    ResourceId m_bound_shader{0};
    ResourceIds m_bound_textures{0};
    uint8_t m_active_texture_unit{0};
    // Is index buffer enabled either via bound IB or VAO
    bool m_bound_indices{false};
};
}