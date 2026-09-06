// Compiles only when BOSS_FEATURES_DIR points at
// src/boss/include/boss/test-fixtures. Proves the real Fill.cpp dispatch
// points (group_fills()'s boss_pattern resolution, make_fills()'s
// dispatch) route a generator-discovered feature through a real,
// processed Print.
#include <catch2/catch_test_macros.hpp>

#include "boss/generated/BossFillPatternKey.hpp"
#include "boss/test-fixtures/fill-fixture/FillFixtureFeature.hpp"
#include "libslic3r/Fill/Fill.hpp"
#include "libslic3r/Fill/boss/test_fixtures/FillTestFixture.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/LayerRegion.hpp"
#include "libslic3r/Print.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

TEST_CASE("A layer's boss_fill_pattern fixture config actually reaches group_fills()'s boss_pattern", "[boss][fill][fixture]")
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("fill_density").set(Domain::Percentage{20.0});
    config.print.items.opt("boss_fill_pattern").set(Domain::Boss::FillPatternKey::FillFixture);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    bool found_boss_pattern = false;
    for (const Layer *layer : print.get_object(0)->layers()) {
        for (const SurfaceFill &fill : group_fills(*layer)) {
            if (fill.params.boss_pattern) {
                REQUIRE(*fill.params.boss_pattern == Slic3r::Boss::Test::FillFixtureFeature::id);
                found_boss_pattern = true;
            }
        }
    }
    REQUIRE(found_boss_pattern);
}

TEST_CASE("A native Lightning fill_pattern alongside the fixture's boss_fill_pattern does not crash make_fills()", "[boss][fill][fixture]")
{
    // fill_pattern and boss_fill_pattern are separate, independently-set
    // config options -- a user can set both on the same region. Before
    // Fill.cpp's dispatch-priority guards (make_fills()'s dynamic_cast
    // sites), the BOSS-dispatched fixture Fill would be dynamic_cast to
    // FillLightning::Filler*, get nullptr, and dereference it -- UB a full
    // print() run reliably crashes on. This is the pipeline-level proof
    // those guards actually work against a real, generator-discovered
    // fixture feature, not just the registry template in isolation.
    //
    // top_solid_layers/bottom_solid_layers are forced to 0 so every layer
    // is pure sparse infill -- the real proof reads fills(), the
    // generated extrusion collection make_fills() actually writes to, not
    // fill_surfaces() (input classification, set before make_fills() ever
    // runs), which would stay non-empty even if dispatch were broken.
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("fill_density").set(Domain::Percentage{20.0});
    config.print.items.opt("top_solid_layers").set(0);
    config.print.items.opt("bottom_solid_layers").set(0);
    config.print.items.opt("fill_pattern").set(Domain::InfillPattern::ipLightning);
    config.print.items.opt("boss_fill_pattern").set(Domain::Boss::FillPatternKey::FillFixture);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    bool found_non_empty_fills = false;
    for (const Layer *layer : print.get_object(0)->layers())
        if (! layer->get_region(0)->fills().empty())
            found_non_empty_fills = true;
    REQUIRE(found_non_empty_fills);
}
