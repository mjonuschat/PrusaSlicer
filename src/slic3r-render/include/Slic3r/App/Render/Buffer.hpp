#pragma once

#include <memory>

#include <Slic3r/Log.hpp>

#include "Types.hpp"
#include "Shader.hpp"
#include "Context.hpp"
#include "WithInternal.hpp"

namespace Slic3r::App::Render {

class Device;

class Buffer : public WithInternal {
public:
    Buffer(Device& device, BufferTarget target);
    ~Buffer() override;

    void set_data(const void* data, size_t size, BufferUsage usage);

    BufferTarget target() const { return m_target; }

protected:
    Device& m_device;
private:
    BufferTarget m_target;
};

class VertexBuffer : public Buffer
{
public:
    explicit VertexBuffer(Device& device) : Buffer(device, BufferTarget::VertexBuffer) {}
};

class IndexBuffer : public Buffer
{
public:
    explicit IndexBuffer(Device& device) : Buffer(device, BufferTarget::IndexBuffer) {}
};

#if !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)
class TextureBuffer : public Buffer
{
public:
    explicit TextureBuffer(Device& device);
    ~TextureBuffer() override;
};
#endif // !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)

} // namespace Slic3r::App::Render
