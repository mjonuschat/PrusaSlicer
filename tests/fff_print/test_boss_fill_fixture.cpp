// Compiles only when BOSS_FEATURES_DIR points at
// src/boss/include/boss/test-fixtures. Proves the real Fill.cpp dispatch
// points route a generator-discovered feature through a real, processed Print.
#include <catch2/catch_test_macros.hpp>

#include "boss/test-fixtures/fill-fixture/FillFixtureFeature.hpp"
#include "libslic3r/Fill/Fill.hpp"
#include "libslic3r/Fill/boss/test_fixtures/FillTestFixture.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/LayerRegion.hpp"
#include "libslic3r/Print.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

namespace {

Domain::InfillPattern fixture_pattern()
{
    return Domain::InfillPattern(Slic3r::Boss::Test::FillFixtureFeature::id);
}

} // namespace

TEST_CASE("A layer's BOSS fill_pattern choice reaches group_fills()", "[boss][fill][fixture]")
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("fill_density").set(Domain::Percentage{20.0});
    config.print.items.opt("fill_pattern").set(fixture_pattern());

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    bool found_fixture_pattern = false;
    for (const Layer *layer : print.get_object(0)->layers())
        for (const SurfaceFill &fill : group_fills(*layer))
            if (fill.params.pattern == fixture_pattern())
                found_fixture_pattern = true;

    REQUIRE(found_fixture_pattern);
}

TEST_CASE("A BOSS fill pattern survives the whole make_fills() pipeline", "[boss][fill][fixture]")
{
    // make_fills() dynamic_casts the filler at its Lightning and Adaptive
    // sites, keyed on params.pattern. A BOSS pattern must match none of them:
    // a match would cast the registry's filler to FillLightning::Filler*, get
    // nullptr and dereference it. Reads fills(), the extrusions make_fills()
    // writes, not fill_surfaces(), which is input classification and stays
    // non-empty even when dispatch is broken.
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("fill_density").set(Domain::Percentage{20.0});
    // Every layer pure sparse infill, so the fills read below are the
    // fixture's own.
    config.print.items.opt("top_solid_layers").set(0);
    config.print.items.opt("bottom_solid_layers").set(0);
    config.print.items.opt("fill_pattern").set(fixture_pattern());

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    bool found_non_empty_fills = false;
    for (const Layer *layer : print.get_object(0)->layers())
        if (! layer->get_region(0)->fills().empty())
            found_non_empty_fills = true;

    REQUIRE(found_non_empty_fills);
}
