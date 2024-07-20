#pragma once

#include <memory>

#include <Slic3r/Log.hpp>

#include "commonGL.hpp"
#include "Shader.hpp"
#include "Context.hpp"
#include "WithInternal.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::App::Render {
enum class BufferTarget {
    VertexBuffer,
    IndexBuffer
};

class Buffer : public WithInternal {
public:
    explicit Buffer(BufferTarget target);
    ~Buffer() override;

    void bind() const;
    void set_data(const void* data, GLsizeiptr size, GLenum usage);

private:
    BufferTarget m_target;
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer() : Buffer(BufferTarget::VertexBuffer) {}
};

class IndexBuffer : public Buffer
{
public:
    IndexBuffer() : Buffer(BufferTarget::IndexBuffer) {}
};

} // namespace Slic3r::App::Render
