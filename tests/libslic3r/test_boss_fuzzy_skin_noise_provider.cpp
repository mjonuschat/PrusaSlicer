#include <cmath>

#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Point.hpp"
#include "libslic3r/boss/surface/fuzzyskin/FuzzySkinNoiseProvider.hpp"

using Slic3r::Boss::FuzzySkinNoiseProvider;
using Slic3r::Domain::Boss::FuzzySkinNoiseType;

namespace {

void check_bounded(FuzzySkinNoiseType type)
{
    const double thickness = 0.3;
    const Slic3r::Point sample{Slic3r::scaled<coord_t>(1.5), Slic3r::scaled<coord_t>(-2.25)};

    for (double slice_z : {0.0, 0.2, 5.0}) {
        const double displacement = FuzzySkinNoiseProvider::get_displacement(type, 1.0, 4, 0.5, sample, slice_z, thickness);
        CHECK(std::isfinite(displacement));
        CHECK(displacement >= -thickness);
        CHECK(displacement <= thickness);
    }
}

} // namespace

TEST_CASE("FuzzySkinNoiseProvider produces bounded, finite displacement for every noise type", "[boss][surface]")
{
    SECTION("Perlin") { check_bounded(FuzzySkinNoiseType::Perlin); }
    SECTION("Billow") { check_bounded(FuzzySkinNoiseType::Billow); }
    SECTION("RidgedMulti") { check_bounded(FuzzySkinNoiseType::RidgedMulti); }
    SECTION("Voronoi") { check_bounded(FuzzySkinNoiseType::Voronoi); }
}

TEST_CASE("FuzzySkinNoiseProvider is deterministic for a fixed sample and slice_z", "[boss][surface]")
{
    const Slic3r::Point sample{Slic3r::scaled<coord_t>(3.7), Slic3r::scaled<coord_t>(4.1)};
    const double a = FuzzySkinNoiseProvider::get_displacement(FuzzySkinNoiseType::Perlin, 1.0, 4, 0.5, sample, 1.2, 0.3);
    const double b = FuzzySkinNoiseProvider::get_displacement(FuzzySkinNoiseType::Perlin, 1.0, 4, 0.5, sample, 1.2, 0.3);
    CHECK(a == b);
}

TEST_CASE("FuzzySkinNoiseProvider varies displacement across slice_z (coherent noise flows vertically)", "[boss][surface]")
{
    const Slic3r::Point sample{Slic3r::scaled<coord_t>(3.7), Slic3r::scaled<coord_t>(4.1)};
    const double a = FuzzySkinNoiseProvider::get_displacement(FuzzySkinNoiseType::Perlin, 1.0, 4, 0.5, sample, 0.0, 0.3);
    const double b = FuzzySkinNoiseProvider::get_displacement(FuzzySkinNoiseType::Perlin, 1.0, 4, 0.5, sample, 5.0, 0.3);
    CHECK(a != b);
}

TEST_CASE("FuzzySkinNoiseProvider reconfigures cleanly when parameters change between calls", "[boss][surface]")
{
    const Slic3r::Point sample{Slic3r::scaled<coord_t>(1.0), Slic3r::scaled<coord_t>(1.0)};
    for (FuzzySkinNoiseType type : {FuzzySkinNoiseType::Perlin, FuzzySkinNoiseType::Voronoi,
                                     FuzzySkinNoiseType::Billow, FuzzySkinNoiseType::RidgedMulti}) {
        const double displacement = FuzzySkinNoiseProvider::get_displacement(type, 2.0, 3, 0.4, sample, 0.6, 0.3);
        CHECK(std::isfinite(displacement));
    }
}
