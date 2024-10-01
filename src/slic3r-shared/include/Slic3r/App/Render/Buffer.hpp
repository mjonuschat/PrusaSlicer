#pragma once

#include <memory>

#include <Slic3r/Log.hpp>

#include "Types.hpp"
#include "Shader.hpp"
#include "Context.hpp"
#include "WithInternal.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::App::Render {

class Device;

class Buffer : public WithInternal {
public:
    Buffer(Device& device, BufferTarget target);
    ~Buffer() override;

    void set_data(const void* data, size_t size, BufferUsage usage);

private:
    Device& m_device;
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

} // namespace Slic3r::App::Render
