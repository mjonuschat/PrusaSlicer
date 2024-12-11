#pragma once

#include "CommandBuffer.hpp"

#include <vector>

#include "Slic3r/App/Render/Geometry.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"

namespace Slic3r::App::Render {

namespace Internal {

#define DEFINE_HAS_ATTR_OF_TYPE_TRAIT(trait, attr_type, attr) \
    template <typename T, typename = void> \
    struct trait : std::false_type {}; \
    template <typename T> \
    struct trait<T, std::void_t<decltype(std::declval<T>().attr)>> \
        : std::is_same<decltype(std::declval<T>().attr), attr_type> {};

DEFINE_HAS_ATTR_OF_TYPE_TRAIT(HasPosition3, Vec3f, position)
DEFINE_HAS_ATTR_OF_TYPE_TRAIT(HasPosition2, Vec2f, position)
DEFINE_HAS_ATTR_OF_TYPE_TRAIT(HasNormal, Vec3f, normal)
DEFINE_HAS_ATTR_OF_TYPE_TRAIT(HasTexCoord2, Vec2f, color)

}

template <typename V>
class DynamicGeometry
{
public:
    using Vertices = std::vector<V>;

    class PrimitiveBuilder
    {
    public:
        PrimitiveBuilder(DynamicGeometry& parent, PrimitiveType primitive)
            : m_parent(parent), m_primitive(primitive), m_vertex(parent.m_vertex), m_offset(parent.m_vertices.size())
        {}

        ~PrimitiveBuilder()
        {
            if (m_vertices.empty())
                return;

            m_parent.m_vertices.insert(
                m_parent.m_vertices.end(),
                std::make_move_iterator(m_vertices.begin()),
                std::make_move_iterator(m_vertices.end())
            );
            m_parent.m_draw_commands.emplace_back({m_primitive, m_offset, m_count, m_parent.m_material});
            m_parent.m_uploaded = false;
        }

        template <typename T>
        PrimitiveBuilder& vertex(const T& v)
        {
            set_position(v);
            m_vertices.push_back(m_vertex);
            return *this;
        }

        PrimitiveBuilder& attr(std::function<void(V&)> modifier)
        {
            modifier(m_vertex);
            return *this;
        }

        template <typename U = V, typename = std::enable_if_t<Internal::HasNormal<U>::value>>
        PrimitiveBuilder& normal(const Vec3f& n)
        {
            m_vertex.normal = n;
            return *this;
        }

        template <typename U = V, typename = std::enable_if_t<Internal::HasTexCoord2<U>::value>>
        PrimitiveBuilder& tex_coord(const Vec2f& tc)
        {
            m_vertex.tex_coord = tc;
            return *this;
        }
    private:
        template <typename U = V, typename = std::enable_if_t<Internal::HasPosition3<U>::value>>
        void set_position(const Vec3f& v)
        {
            m_vertex.position = v;
        }

        template <typename U = V, typename = std::enable_if_t<Internal::HasPosition2<U>::value>>
        void set_position(const Vec2f& v)
        {
            m_vertex.position = v;
        }

    private:
        DynamicGeometry& m_parent;
        PrimitiveType m_primitive;
        V m_vertex;
        Vertices m_vertices;
        size_t m_offset;
        size_t m_count{0};
    };

    explicit DynamicGeometry(Device& device, BufferUsage usage = BufferUsage::DynamicDraw)
        : m_device(device), m_usage(usage), m_geometry(device, m_usage)
    {}

    void upload()
    {
        if (m_uploaded)
            return;

        m_geometry.upload(&m_vertices[0], m_vertices.size(), V::format());
        m_geometry.draw_commands() = m_draw_commands;
        m_uploaded = true;

        clear();
    }

    void clear()
    {
        const size_t data_size = m_vertices.size() * sizeof(V);
        m_vertices.clear();
        m_draw_commands.clear();
        if (data_size > MAX_TRANSIENT_DATA_SIZE) {
            m_vertices.shrink_to_fit();
            m_draw_commands.shrink_to_fit();
        }
    }

    void draw(CommandBuffer& cmd_buffer, const Material& material = {})
    {
        if (!m_uploaded)
            upload();
        cmd_buffer.bind_and_draw(m_geometry, material);
    }

    PrimitiveBuilder build_primitive(PrimitiveType primitive)
    { return PrimitiveBuilder(*this, primitive); }


private:
    friend class PrimitiveBuilder;

    Device& m_device;
    BufferUsage m_usage;
    Vertices m_vertices;
    DrawCommands m_draw_commands;
    Geometry m_geometry;

    bool m_uploaded{false};

    // template for building elements
    Material m_material;
    V m_vertex;

    // Max size of vertex data to keep after upload
    static constexpr size_t MAX_TRANSIENT_DATA_SIZE = 655536;
};

}
