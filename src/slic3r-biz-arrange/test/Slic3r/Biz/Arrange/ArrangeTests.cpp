#include <catch2/catch_test_macros.hpp>
#include <random>
#include <numeric>
#include <numbers>
#include "Slic3r/Biz/Algorithms/ClipperUtils.hpp"
#include "Slic3r/Biz/Algorithms/Geometry/Circle.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Biz/Arrange/Arrange.hpp"
#include "Slic3r/Biz/Algorithms/SVG.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"

using Slic3r::Biz::Algorithms::ClipperUtils::intersection;
using Slic3r::Biz::Algorithms::ClipperUtils::offset;
using Slic3r::Biz::Algorithms::Geometry::Circle;
using Slic3r::Biz::Algorithms::Scaling::scaled;
using Slic3r::Biz::Arrange::ArbitraryShape;
using Slic3r::Biz::Arrange::arrange;
using Slic3r::Biz::Arrange::ArrangeItem;
using Slic3r::Biz::Arrange::ArrangeResult;
using Slic3r::Biz::Arrange::CircleBed;
using Slic3r::Biz::Arrange::ConvexShape;
using Slic3r::Biz::Arrange::ConvexShapes;
using Slic3r::Biz::Arrange::PivotPoint;
using Slic3r::Biz::Arrange::RectangleBed;
using Slic3r::Biz::Arrange::Segments;
using Slic3r::Biz::Arrange::Settings;
using Slic3r::Domain::BoundingBox2crd;
using Slic3r::Domain::coord_t;
using Slic3r::Domain::ExPolygon;
using Slic3r::Domain::Polygon;
using Slic3r::Domain::Polygons;
using Slic3r::Domain::Vec2crd;
using Slic3r::Domain::Vec2d;

#ifndef NDEBUG
constexpr bool output_result_svgs{true};
#else
constexpr bool output_result_svgs{false};
#endif

ArbitraryShape get_square(coord_t scaled_size)
{
    return {{
        Vec2crd{0, 0},
        Vec2crd{scaled_size, 0},
        Vec2crd{scaled_size, scaled_size},
        Vec2crd{0, scaled_size},
    }};
}

namespace {
template <typename T>
T random_value(const T min, const T max, std::mt19937& gen)
{
    if constexpr (std::is_integral_v<T>) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(gen);
    } else if constexpr (std::is_same_v<T, Vec2crd>) {
        std::uniform_int_distribution<coord_t> x_dist(min.x(), max.x());
        std::uniform_int_distribution<coord_t> y_dist(min.y(), max.y());
        return Vec2crd{x_dist(gen), y_dist(gen)};
    } else {
        std::uniform_real_distribution<T> dist(min, max);
        return dist(gen);
    }
}

double area(const ArbitraryShape& shape)
{
    return std::accumulate(shape.begin(), shape.end(), 0.0, [](double result, const ExPolygon& poly) {
        return result + poly.area();
    });
}

std::vector<ArbitraryShape> get_random_squares(const std::size_t cube_count, const unsigned seed)
{
    std::mt19937 gen{seed};
    const BoundingBox2crd inital_limits{scaled(Vec2d{0.0, 0.0}), scaled(Vec2d{100.0, 100.0})};

    std::vector<ArbitraryShape> result;
    for (size_t i = 0; i < cube_count; ++i) {
        ArbitraryShape square{get_square(random_value(scaled(5.0), scaled(20.0), gen))};
        square.front().translate(random_value(inital_limits.min, inital_limits.max, gen));
        square.front().rotate(random_value(0.0, 2 * std::numbers::pi, gen));
        result.push_back(std::move(square));
    }

    std::ranges::sort(result, [](const ArbitraryShape& a, const ArbitraryShape& b) {
        return area(a) > area(b);
    });

    return result;
}

bool is_collision_free(const std::vector<ArrangeItem>& items)
{
    using namespace Slic3r;

    for (std::size_t i{}; i < items.size(); ++i) {
        for (std::size_t j{i + 1}; j < items.size(); j++) {
            const ArrangeItem& item1{items[i]};
            const ArrangeItem& item2{items[j]};

            const Polygons outline1{offset(
                item1.fixed_shape().transformed_outline(),
                -static_cast<float>(Domain::SCALED_EPSILON)
            )};
            const Polygons outline2{offset(
                item2.fixed_shape().transformed_outline(),
                -static_cast<float>(Domain::SCALED_EPSILON)
            )};

            if (!intersection(outline1, outline2).empty()) {
                return false;
            }
        }
    }

    return true;
}

void check_arrange_result(const ArrangeResult& result, const std::size_t items_count)
{
    CHECK(result.packed.size() == items_count);
    CHECK(result.not_packed.empty());
    CHECK(is_collision_free(result.packed));
}
} // namespace

void draw(std::string_view filename, const std::vector<ArrangeItem>& items, const BoundingBox2crd bed_bb)
{
    using Slic3r::Biz::Algorithms::Polygon::to_expolygons;
    using Slic3r::Biz::Algorithms::SVG::SVG;
    using Slic3r::Domain::ExPolygon;
    using Slic3r::Domain::ExPolygons;

    SVG svg{filename.data(), bed_bb};

    const Polygon bounding_box_poly{
        bed_bb.min,
        Vec2crd{bed_bb.max.x(), bed_bb.min.y()},
        bed_bb.max,
        Vec2crd{bed_bb.min.x(), bed_bb.max.y()}
    };
    svg.draw(ExPolygon{bounding_box_poly}, "blue", .2f);

    for (const ArrangeItem& item : items) {
        const ExPolygons fixed_shape{to_expolygons(item.fixed_shape().transformed_outline())};
        svg.draw(fixed_shape, "yellow", .5f);
        svg.draw_outline(fixed_shape);
    }
}

TEST_CASE("Arrange without rotation works on random squares on a rectangular bed", "[Arrange][Integration]")
{
    const std::vector<ArbitraryShape> squares{get_random_squares(20, 0)};
    const RectangleBed bed{{scaled(Vec2d{0.0, 0.0}), scaled(Vec2d{100, 100})}};
    const ArrangeResult result{arrange(bed, squares, {}, Settings{})};

    if constexpr (output_result_svgs) {
        draw("rectangular_bed_no_rotation.svg", result.packed, bed.bounding_box());
    }

    CHECK(std::ranges::all_of(result.packed, [&bed](const ArrangeItem& item) {
        return bed.bounding_box().contains(item.movable_shape().bounding_box());
    }));
}

namespace {
bool circle_contains(const Circle<Vec2d>& circle, const ArrangeItem& item)
{
    for (const ConvexShape& shape : item.fixed_shape().transformed_outline()) {
        for (const Vec2crd& point : shape) {
            if (!circle.contains(point.cast<double>())) {
                return false;
            }
        }
    }
    return true;
}
} // namespace

TEST_CASE("Arrange without rotation works on random squares on a circular bed", "[Arrange][Integration]")
{
    const std::vector<ArbitraryShape> squares{get_random_squares(20, 0)};

    const Vec2crd center{scaled(Vec2d{0.0, 0.0})};
    const Circle<Vec2d> circle{center.cast<double>(), scaled(50.0)};
    const CircleBed bed{center, circle.radius};

    const ArrangeResult result{arrange(bed, squares, {}, Settings{})};

    if constexpr (output_result_svgs) {
        draw("circle_bed_no_rotation.svg", result.packed, bed.bounding_box());
    }

    check_arrange_result(result, squares.size());

    CHECK(std::ranges::all_of(result.packed, [&circle](const ArrangeItem& item) {
        return circle_contains(circle, item);
    }));
}

TEST_CASE("Arrange with rotation works on random squares on a rectangular bed", "[Arrange][Integration]")
{
    const std::vector<ArbitraryShape> squares{get_random_squares(10, 0)};
    const RectangleBed bed{{scaled(Vec2d{0.0, 0.0}), scaled(Vec2d{60, 60})}};

    Settings settings;
    settings.allow_rotations   = true;
    settings.arrangment_limits = bed.bounding_box();
    const ArrangeResult result{arrange(bed, squares, {}, settings)};

    if constexpr (output_result_svgs) {
        draw("rectangular_bed_with_rotation.svg", result.packed, bed.bounding_box());
    }

    check_arrange_result(result, squares.size());

    CHECK(std::ranges::all_of(result.packed, [&bed](const ArrangeItem& item) {
        return bed.bounding_box().contains(item.movable_shape().bounding_box());
    }));
}

TEST_CASE("Arrange with rotation works on random squares on a circular bed", "[Arrange][Integration]")
{
    std::vector<ArbitraryShape> squares{get_random_squares(8, 0)};

    const Vec2crd center{scaled(Vec2d{0.0, 0.0})};
    const Circle<Vec2d> circle{center.cast<double>(), scaled(30.0)};
    const CircleBed bed{center, circle.radius};

    Settings settings;
    settings.allow_rotations = true;
    ArrangeResult result{arrange(bed, squares, {}, settings)};

    if constexpr (output_result_svgs) {
        draw("circle_bed_with_rotation.svg", result.packed, bed.bounding_box());
    }

    check_arrange_result(result, squares.size());

    CHECK(std::ranges::all_of(result.packed, [&circle](const ArrangeItem& item) {
        return circle_contains(circle, item);
    }));
}

TEST_CASE("Arrange works with offset", "[Arrange][Integration]")
{
    const std::vector<ArbitraryShape> squares{get_square(scaled(10.0)), get_square(scaled(10.0))};

    const RectangleBed bed{{scaled(Vec2d{0.0, 0.0}), scaled(Vec2d{50.0, 30.0})}};

    Settings settings;
    settings.scaled_offset = scaled(5.0);
    const ArrangeResult result{arrange(bed, squares, {}, settings)};

    if constexpr (output_result_svgs) {
        draw("offset.svg", result.packed, bed.bounding_box());
    }

    check_arrange_result(result, squares.size());

    CHECK(result.packed.at(0).get_translation() == Vec2crd{scaled(30.0), scaled(10.0)});
    CHECK(result.packed.at(1).get_translation() == Vec2crd{scaled(10.0), scaled(10.0)});
}

TEST_CASE("Arrange works with segmented bed and pivot", "[Arrange][Integration]")
{
    const std::vector<ArbitraryShape> squares{get_square(scaled(40.0))};

    const RectangleBed bed{
        {scaled(Vec2d{0.0, 0.0}), scaled(Vec2d{100.0, 100.0})},
        PivotPoint::BottomRight,
        Segments{4, 4},
    };

    const ArrangeResult result{arrange(bed, squares, {}, Settings{})};

    if constexpr (output_result_svgs) {
        draw("segmented_bed.svg", result.packed, bed.bounding_box());
    }

    check_arrange_result(result, squares.size());

    CHECK(result.packed.at(0).get_translation() == Vec2crd{scaled(55.0), scaled(5.0)});
}
