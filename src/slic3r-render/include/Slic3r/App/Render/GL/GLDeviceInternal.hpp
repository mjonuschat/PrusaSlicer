#pragma once

#include <vector>
#include <unordered_set>
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

    void bind_shader(const Shader& s);
    void bind_buffer(BufferTarget target, ResourceId buffer);
    void unbind_buffer(BufferTarget target);
    void bind_geometry(const Geometry& g, const Shader& s);
    void unbind_geometry();
    void bind_texture(uint8_t unit, const Texture& t);
    void unbind_texture(uint8_t unit, const Texture& t);
#if !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)
    void bind_texture_buffer_texture(uint8_t unit, ResourceId texture_buffer);
    void unbind_texture_buffer_texture(uint8_t unit);
#endif // !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)

    void* map_buffer(BufferTarget target, BufferAccess access);
    void unmap_buffer(BufferTarget target);

    void draw(PrimitiveType primitive, size_t offset, size_t count);

    void print_buffer_info(const char* action = nullptr);
private:
    friend class ::Slic3r::App::Render::Geometry;
    void activate_texture_unit(uint8_t unit);
    void bind_vertex_buffer(ResourceId vb);
    void bind_index_buffer(ResourceId ib);
#if !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)
    void bind_texture_buffer(ResourceId tb);
#endif // !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)
    void bind_vao(ResourceId vao);

private:
    Context& m_context;
    // only active bound VB (not taking bound VAO into account)
    ResourceId m_bound_vertex_buffer{0};
    // only active bound IB (not taking bound VAO into account)
    ResourceId m_bound_index_buffer{0};
#if !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)
    // active bound TB
    ResourceId m_bound_texture_buffer{ 0 };
#endif // !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)
    IndexType m_bound_index_type{IndexType::UByte};
    ResourceId m_bound_vao{0};
    ResourceId m_bound_shader{0};
    ResourceIds m_bound_textures;
    uint8_t m_active_texture_unit{0};
    // Is index buffer enabled either via bound IB or VAO
    bool m_bound_indices{false};
};
}