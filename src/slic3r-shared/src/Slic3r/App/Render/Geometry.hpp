#pragma once

#include "Types.hpp"
#include "Buffer.hpp"
#include "VertexAttribDesc.hpp"
#include "WithInternal.hpp"

namespace Slic3r::App::Render {

class Device;

class Geometry : public WithInternal
{
public:
    explicit Geometry(Device& device);
    ~Geometry() override;

    Geometry(Geometry&&) = default;

    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;

    void upload(
        const void* vertex_data,
        size_t vertex_count,
        const VertexAttribsDesc& vertex_format,
        const void* index_data = nullptr,
        size_t index_count = 0,
        IndexType index_format = IndexType::UInt
    );

    VertexBuffer* vertex_buffer() { return m_vb.get(); }
    const VertexBuffer* vertex_buffer() const { return m_vb.get(); }

    IndexBuffer* index_buffer() { return m_ib.get(); }
    const IndexBuffer* index_buffer() const { return m_ib.get(); }

    const VertexAttribsDesc& vertex_format() const { return m_vertex_format; }
    IndexType index_type() const { return m_index_type; }

    bool ready() const { return m_built; }

private:
    Device& m_device;
    std::unique_ptr<VertexBuffer> m_vb;
    std::unique_ptr<IndexBuffer> m_ib;

    VertexAttribsDesc m_vertex_format;
    IndexType m_index_type{IndexType::UInt};
    size_t m_vertex_count{0};
    size_t m_index_count{0};

    bool m_built{false};
};

}