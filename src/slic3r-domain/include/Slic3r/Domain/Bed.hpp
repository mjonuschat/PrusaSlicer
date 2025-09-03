#pragma once

#include <optional>
#include <string_view>

#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"

namespace Slic3r::Domain {

enum class BedType
{
    // Not set yet or undefined.
    Invalid,
    // Rectangular print bed. Most common, cheap to work with.
    Rectangle,
    // Circular print bed. Common on detals, cheap to work with.
    Circle,
    // Convex print bed. Complex to process.
    Convex,
    // Some non convex shape.
    Custom
};

inline std::string_view bed_type_name(BedType type)
{
    using namespace std::literals;
    switch (type) {
    default:
    case BedType::Invalid:
        return "Invalid"sv;
    case BedType::Rectangle:
        return "Rectangle"sv;
    case BedType::Circle:
        return "Circle"sv;
    case BedType::Convex:
        return "Convex"sv;
    case BedType::Custom:
        return "Custom"sv;
    }
}

class Bed : public ObjectBase
{
public:
    struct Segments
    {
        std::size_t x_count{1};
        std::size_t y_count{1};
    };

    [[nodiscard]] static Bed
    from(const Vec2ds& contour, float max_print_height, const std::optional<Segments>& bed_segments, const std::string& model_filename, const std::string& texture_filename);

    [[nodiscard]] BedType type() const
    {
        return m_type;
    }

    void set_type(BedType type)
    {
        m_type = type;
    }

    [[nodiscard]] const Vec2d& center() const
    {
        return m_center;
    }

    [[nodiscard]] const Vec2d& offset() const
    {
        return m_offset;
    }

    [[nodiscard]] const Vec2ds& contour() const
    {
        return m_contour;
    }

    [[nodiscard]] const Vec2d& contour_aabb_extent() const
    {
        return m_contour_aabb_extent;
    }

    [[nodiscard]] float max_print_height() const
    {
        return m_max_print_height;
    }

    [[nodiscard]] std::optional<Segments> segments() const
    {
        return m_segments;
    }

    using TopBottomDecomposition = std::pair<Vec2ds, Vec2ds>;

    [[nodiscard]] std::optional<TopBottomDecomposition> top_bottom_convex_hull_decomposition() const
    {
        return m_top_bottom_convex_hull_decomposition;
    }

    void set_top_bottom_convex_hull_decomposition(const TopBottomDecomposition& decomposition)
    {
        m_top_bottom_convex_hull_decomposition = decomposition;
    }

    using Circle = std::pair<Vec2d, double>;

    [[nodiscard]] std::optional<Circle> circle() const
    {
        return m_circle;
    }

    void set_circle(const Circle& circle)
    {
        m_circle = circle;
    }

    [[nodiscard]] const std::string& model_filename() const
    {
        return m_model_filename;
    }

    [[nodiscard]] const std::string& texture_filename() const
    {
        return m_texture_filename;
    }

    bool operator==(const Bed& rhs) const;

private:
    BedType m_type{BedType::Invalid};
    Vec2d m_center{Vec2d::Zero()};
    Vec2d m_offset{Vec2d::Zero()};
    Vec2ds m_contour;
    Vec2d m_contour_aabb_extent{Vec2d::Zero()};
    float m_max_print_height{0.0f};
    std::string m_model_filename;
    std::string m_texture_filename;
    std::optional<Segments> m_segments;
    std::optional<TopBottomDecomposition> m_top_bottom_convex_hull_decomposition;
    std::optional<Circle> m_circle;
};

bool operator==(const Bed::Segments& a, const Bed::Segments& b);

} // namespace Slic3r::Domain
