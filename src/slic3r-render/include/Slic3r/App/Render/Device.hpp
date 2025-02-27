#pragma once

#include <vector>
#include <cstddef>

#include "WithInternal.hpp"
#include "Types.hpp"

namespace Slic3r::App::Render {

class Context;
class Texture;
class Buffer;
class VertexBuffer;
class IndexBuffer;
#ifdef SLIC3R_RENDER_TEXTURE_BUFFER_SUPPORTED
class TextureBuffer;
#endif // SLIC3R_RENDER_TEXTURE_BUFFER_SUPPORTED
class Shader;
class CommandBuffer;

class Device : public WithInternal
{
    friend class Context;
    explicit Device(Context& context);

    Device(Device&&) = default;
public:
    Context& context() { return m_context; }
    const Context& context() const { return m_context; }

    void load_state();

    std::unique_ptr<Texture> create_texture();
    std::unique_ptr<VertexBuffer> create_vertex_buffer();
    std::unique_ptr<IndexBuffer> create_index_buffer();
#ifdef SLIC3R_RENDER_TEXTURE_BUFFER_SUPPORTED
    std::unique_ptr<TextureBuffer> create_texture_buffer(PixelFormat format);
#endif // SLIC3R_RENDER_TEXTURE_BUFFER_SUPPORTED
    std::unique_ptr<CommandBuffer> create_command_buffer();

    void bind_buffer(const Buffer& b);
    void unbind_buffer(const Buffer& b);

    void* map_buffer(const Buffer& b, BufferAccess access);
    void unmap_buffer(const Buffer& b);

private:
    Context& m_context;
};

}
