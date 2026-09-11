#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <Slic3r/Biz/GCodeReader/GCodeReader.hpp>

#include "boss/features/z-hop-surface-filter/ZHopSurfaceFilterFeature.hpp"
#include "libslic3r/GCode/GCodeWriter.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;
using Biz::GCodeReader::GCodeReader;
using Slic3r::Boss::ZHopSurfaceFilterMode;

namespace {

constexpr double kRetractLift = 0.6;
constexpr double kLayerHeight = 0.2;

struct LiftCounts
{
    unsigned first_layer  = 0;
    unsigned second_layer = 0;
};

// self.z() is the print Z before the move fires, so it sorts a lift move into
// the first layer's band or the second's.
LiftCounts count_lift_moves(const std::string& gcode)
{
    LiftCounts counts;
    GCodeReader parser;
    parser.parse_buffer(
        gcode,
        [&](GCodeReader& self, const GCodeReader::GCodeLine& line)
        {
            if (line.dist_Z(self) != Catch::Approx(kRetractLift))
                return;
            if (self.z() < 1.5 * kLayerHeight)
                ++counts.first_layer;
            else
                ++counts.second_layer;
        }
    );
    return counts;
}

// A 10x10x0.4mm block sliced at 0.2mm layers gives exactly two layers: layer
// 0 is the first layer and a bottom surface (not top), layer 1 is neither
// the first layer nor a bottom surface -- it is the object's top.
std::string slice_with_mode(ZHopSurfaceFilterMode mode)
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("layer_height").set(kLayerHeight);
    config.print.items.opt("first_layer_height").set(Domain::FloatOrPercentage{kLayerHeight});
    config.print.items.opt("top_solid_layers").set(1);
    config.print.items.opt("bottom_solid_layers").set(1);
    config.print.items.opt("perimeters").set(2);
    config.print.items.opt("fill_density").set(Domain::Percentage{0.0});
    config.print.items.opt("retract_length").set(1.0);
    config.print.items.opt("retract_before_travel").set(0.0);
    config.print.items.opt("retract_lift").set(kRetractLift);
    config.print.items.opt("retract_lift_enforce").set(mode);

    return Slic3r::Test::slice(
        {Slic3r::Test::mesh(TestMesh::cube_20x20x20, Vec3d(0, 0, 0), Vec3d(0.5, 0.5, 0.02))},
        config
    );
}

} // namespace

TEST_CASE("ZHopSurfaceFilterFeature all_surfaces lifts on every layer", "[boss][zhop]")
{
    LiftCounts counts = count_lift_moves(slice_with_mode(ZHopSurfaceFilterMode::AllSurfaces));
    CHECK(counts.first_layer > 0);
    CHECK(counts.second_layer > 0);
}

TEST_CASE("ZHopSurfaceFilterFeature top_only only lifts over the top surface", "[boss][zhop]")
{
    LiftCounts counts = count_lift_moves(slice_with_mode(ZHopSurfaceFilterMode::TopOnly));
    CHECK(counts.first_layer == 0);
    CHECK(counts.second_layer > 0);
}

TEST_CASE("ZHopSurfaceFilterFeature bottom_only only lifts on the first layer", "[boss][zhop]")
{
    LiftCounts counts = count_lift_moves(slice_with_mode(ZHopSurfaceFilterMode::BottomOnly));
    CHECK(counts.first_layer > 0);
    CHECK(counts.second_layer == 0);
}

TEST_CASE("ZHopSurfaceFilterFeature top_and_bottom lifts on both layers", "[boss][zhop]")
{
    LiftCounts counts = count_lift_moves(slice_with_mode(ZHopSurfaceFilterMode::TopAndBottom));
    CHECK(counts.first_layer > 0);
    CHECK(counts.second_layer > 0);
}
