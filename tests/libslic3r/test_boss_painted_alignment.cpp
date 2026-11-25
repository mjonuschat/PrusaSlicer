#include <catch2/catch_test_macros.hpp>

#include "boss/features/painted-seam-alignment/PaintedAlignmentFeature.hpp"
#include "libslic3r/GCode/SeamPerimeters.hpp"

using namespace Slic3r;
using namespace Slic3r::Boss;
using namespace Slic3r::Seams;

TEST_CASE("cluster_positions groups nearby points into one centroid", "[boss][seam]")
{
    std::vector<Vec2d> positions{Vec2d(0.0, 0.0), Vec2d(0.5, 0.0), Vec2d(10.0, 10.0)};
    auto clusters = PaintedAlignmentFeature::cluster_positions(positions, 1.0);

    REQUIRE(clusters.size() == 2);
    CHECK(clusters[0] == Vec2d(0.25, 0.0));
    CHECK(clusters[1] == Vec2d(10.0, 10.0));
}

TEST_CASE(
    "cluster_positions returns each point separately when none are within radius", "[boss][seam]"
)
{
    std::vector<Vec2d> positions{Vec2d(0.0, 0.0), Vec2d(100.0, 0.0)};
    auto clusters = PaintedAlignmentFeature::cluster_positions(positions, 1.0);

    CHECK(clusters.size() == 2);
}

TEST_CASE("cluster_positions returns empty for empty input", "[boss][seam]")
{
    CHECK(PaintedAlignmentFeature::cluster_positions({}, 1.0).empty());
}

TEST_CASE(
    "get_enforcer_centroid_near returns nullopt when no enforcer is within max_distance",
    "[boss][seam]"
) {
    Perimeters::Perimeter perimeter{};
    perimeter.positions = {Vec2d(100.0, 100.0)};
    perimeter.point_types = {Perimeters::PointType::enforcer};

    CHECK_FALSE(
        PaintedAlignmentFeature::get_enforcer_centroid_near(perimeter, Vec2d::Zero(), 1.0).has_value());
}

TEST_CASE(
    "get_enforcer_centroid_near averages every enforcer within max_distance",
    "[boss][seam]"
) {
    Perimeters::Perimeter perimeter{};
    perimeter.positions = {Vec2d(0.0, 0.0), Vec2d(1.0, 0.0), Vec2d(100.0, 100.0)};
    perimeter.point_types = {Perimeters::PointType::enforcer, Perimeters::PointType::enforcer,
                              Perimeters::PointType::enforcer};

    auto centroid =
        PaintedAlignmentFeature::get_enforcer_centroid_near(perimeter, Vec2d::Zero(), 2.0);
    REQUIRE(centroid.has_value());
    CHECK(*centroid == Vec2d(0.5, 0.0));
}
