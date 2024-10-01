#pragma once

#include <vector>
#include <cstddef>

#include "WithInternal.hpp"

namespace Slic3r::App::Render {

class Context;
class Texture;
class VertexBuffer;
class IndexBuffer;
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
    std::unique_ptr<CommandBuffer> create_command_buffer();
private:
    Context& m_context;
};

}
