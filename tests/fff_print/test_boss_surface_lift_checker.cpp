// SurfaceLiftChecker's public interface takes a real Layer&, and Layer's
// constructor is private to PrintObject -- there is no way to build one in
// isolation. This test slices a small object instead of constructing a
// fixture Layer directly, which is why it lives here (test_data.hpp's
// slicing helpers) rather than in tests/libslic3r.
#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Layer.hpp"
#include "libslic3r/LayerRegion.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/boss/gcode/retract/SurfaceLiftChecker.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;
using Slic3r::Boss::SurfaceLiftChecker;

namespace {

// A 10x10x0.4mm block sliced at 0.2mm layers gives exactly two layers: layer 0
// rests on the bed (a bottom surface, not top), layer 1 is the object's top
// (a top surface covering the whole 10x10mm footprint).
void slice_two_layer_block(Print& print)
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("layer_height").set(0.2);
    config.print.items.opt("first_layer_height").set(Domain::FloatOrPercentage{0.2});
    config.print.items.opt("top_solid_layers").set(1);
    config.print.items.opt("bottom_solid_layers").set(1);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Domain::Percentage{0.0});

    Slic3r::Test::init_and_process_print(
        {Slic3r::Test::mesh(TestMesh::cube_20x20x20, Vec3d(0, 0, 0), Vec3d(0.5, 0.5, 0.02))},
        print,
        config
    );
}

void slice_two_layer_block_with_hole(Print& print)
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("layer_height").set(0.2);
    config.print.items.opt("first_layer_height").set(Domain::FloatOrPercentage{0.2});
    config.print.items.opt("top_solid_layers").set(1);
    config.print.items.opt("bottom_solid_layers").set(1);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Domain::Percentage{0.0});

    Slic3r::Test::init_and_process_print(
        {Slic3r::Test::mesh(TestMesh::cube_with_hole, Vec3d(0, 0, 0), Vec3d(0.5, 0.5, 0.04))},
        print,
        config
    );
}

} // namespace

struct Slic3r::Boss::SurfaceLiftCheckerCacheTestAccess
{
    static void set_cache_key(SurfaceLiftChecker& checker, const Layer* layer)
    {
        checker.m_layer = layer;
    }
};

TEST_CASE("SurfaceLiftChecker finds a point inside a top surface", "[boss][zhop]")
{
    Print print;
    slice_two_layer_block(print);
    const auto& layers = print.get_object(0)->layers();
    REQUIRE(layers.size() == 2);

    SurfaceLiftChecker checker;
    CHECK(checker.is_over_top_surface(*layers[1], 0.0, 0.0));
}

TEST_CASE("SurfaceLiftChecker returns false for a point outside every top surface", "[boss][zhop]")
{
    Print print;
    slice_two_layer_block(print);
    const auto& layers = print.get_object(0)->layers();
    REQUIRE(layers.size() == 2);

    SurfaceLiftChecker checker;
    CHECK_FALSE(checker.is_over_top_surface(*layers[1], 500.0, 500.0));
}

TEST_CASE("SurfaceLiftChecker rebuilds its cache when the layer pointer changes", "[boss][zhop]")
{
    Print print;
    slice_two_layer_block(print);
    const auto& layers = print.get_object(0)->layers();
    REQUIRE(layers.size() == 2);

    SurfaceLiftChecker checker;
    CHECK_FALSE(checker.is_over_top_surface(*layers[0], 0.0, 0.0));
    CHECK(checker.is_over_top_surface(*layers[1], 0.0, 0.0));
}

TEST_CASE(
    "SurfaceLiftChecker rebuilds when object or layer id differ despite a matching Layer "
    "pointer",
    "[boss][zhop]"
)
{
    // Reproduces the A1 bug: a thread_local checker's Layer* can alias a
    // Layer from an earlier print once the allocator reuses the address.
    Print print_a;
    slice_two_layer_block(print_a);
    const auto& layers_a = print_a.get_object(0)->layers();
    REQUIRE(layers_a.size() == 2);

    Print print_b;
    slice_two_layer_block_with_hole(print_b);
    const auto& layers_b = print_b.get_object(0)->layers();
    REQUIRE(layers_b.size() == 2);

    SurfaceLiftChecker checker;
    CHECK(checker.is_over_top_surface(*layers_a[1], 0.0, 0.0));

    Boss::SurfaceLiftCheckerCacheTestAccess::set_cache_key(checker, layers_b[1]);
    CHECK_FALSE(checker.is_over_top_surface(*layers_b[1], 0.0, 0.0));
}
