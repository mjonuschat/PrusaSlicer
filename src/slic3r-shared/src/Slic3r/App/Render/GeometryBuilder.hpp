#pragma once

#include "Geometry.hpp"

namespace Slic3r::App::Render {

class Device;

struct VertexP3
{
    Vec3f position;

    static const VertexAttribsDesc& format();
};

struct VertexP3N3
{
    Vec3f position;
    Vec3f normal;

    static const VertexAttribsDesc& format();
};

struct VertexP3T2
{
    Vec3f position;
    Vec2f tex_coord;

    static const VertexAttribsDesc& format();
};

struct VertexP3N3T2
{
    Vec3f position;
    Vec3f normal;
    Vec2f tex_coord;

    static const VertexAttribsDesc& format();
};

static_assert(sizeof(VertexP3) == 3 * 4);
static_assert(sizeof(VertexP3N3) == (3 + 3) * 4);
static_assert(sizeof(VertexP3N3T2) == (3 + 3 + 2) * 4);


template <typename T>
struct CommonIndexTypeTraits {
    constexpr static bool allowed = false;
};

template <>
struct CommonIndexTypeTraits<uint8_t> { constexpr static bool allowed = true; };
template <>
struct CommonIndexTypeTraits<uint16_t> { constexpr static bool allowed = true; };
template <>
struct CommonIndexTypeTraits<uint32_t> { constexpr static bool allowed = true; };

template <typename V, typename I=uint32_t>
class GeometryBuilder
{
    static_assert(
        CommonIndexTypeTraits<I>::allowed,
        "I parameters must be one of unsigned int, unsigned short, unsigned char"
    );

public:
    GeometryBuilder() = default;
    GeometryBuilder(GeometryBuilder&&) = default;
    GeometryBuilder(const GeometryBuilder&) = delete;

    using VertexType = V;
    //using IndexType = I;

    GeometryBuilder& add_vertex(const V& v)
    {
        m_vertices.push_back(v);
        return *this;
    }

    GeometryBuilder& add_vertex(const V& v, I& out_idx)
    {
        out_idx = static_cast<I>(m_vertices.size());
        m_vertices.push_back(v);
        return *this;
    }

    GeometryBuilder& add_index(I i)
    {
        m_indices.push_back(i);
        return *this;
    }

    GeometryBuilder& add_triangle_indices(I i0, I i1, I i2)
    {
        m_indices.push_back(i0);
        m_indices.push_back(i1);
        m_indices.push_back(i2);
        return *this;
    }

    std::unique_ptr<Geometry> build(Device& device)
    {
        std::unique_ptr<Geometry> geometry = std::make_unique<Geometry>(device);

        void* index_data = nullptr;
        size_t index_count = 0;
        IndexType index_type = IndexType::UInt;
        std::unique_ptr<char[]> repacked_index_data;

        if (!m_indices.empty()) {
            index_count = m_indices.size();
            if (repack_index<unsigned char>(m_indices, repacked_index_data, index_type) ||
                repack_index<unsigned short>(m_indices, repacked_index_data, index_type))
                index_data = repacked_index_data.get();
            else {
                index_type = IndexTypeTraits<I>::index_type;
                index_data = m_indices.data();
            }
        }

        geometry->upload(
            m_vertices.data(), m_vertices.size(), VertexType::format(),
            index_data, index_count,index_type
        );
        m_vertices.clear();
        m_indices.clear();
        return geometry;
    }
private:
    template <typename DestI>
    static bool repack_index(const std::vector<I>& src, std::unique_ptr<char[]>& dest, IndexType& destType)
    {
        if (IndexTypeTraits<I>::index_type == IndexTypeTraits<DestI>::index_type)
            return false;
        const auto size = src.size();
        if (size > std::numeric_limits<DestI>::max())
            return false;

        destType = IndexTypeTraits<DestI>::index_type;
        auto* dest_data = new char[sizeof(DestI) * size];
        for (size_t i = 0; i < size; i++) {
            DestI* dest_el = reinterpret_cast<DestI*>(&dest_data[sizeof(DestI) * i]);
            *dest_el = static_cast<DestI>(src[i]);
        }
        dest.reset(dest_data);
        return true;
    }

private:
    std::vector<V> m_vertices;
    std::vector<I> m_indices;
};

} // namespace Slic3r::App::Render
