#pragma once

#include <cstddef>
#include "Types.hpp"
#include "WithInternal.hpp"

namespace Slic3r::App::Render {

class Geometry;
class Shader;
class Texture;
class Device;

class CommandBuffer : public WithInternal
{
public:
    explicit CommandBuffer(Device& device);

    void set_clear_values(const RgbaF& clear_color, double clear_depth = 1);
    void clear_buffers(bool color, bool depth);

    void set_viewport(const Rect& viewport);

    void set_scissor(const Rect& scissor);
    void set_scissor_enabled(bool enabled);

    void set_blending(const Blending& blending);
    void set_blending_enabled(bool enabled);

    void bind_shader(Shader& s);
    void bind_geometry(Geometry& g, Shader& s);
    void bind_texture(uint8_t unit, Texture& t);
    void unbind_texture(uint8_t unit, Texture& t);

    void draw(PrimitiveType primitive, size_t offset, size_t count);

    void submit();
private:
    Device& m_device;
};

}

