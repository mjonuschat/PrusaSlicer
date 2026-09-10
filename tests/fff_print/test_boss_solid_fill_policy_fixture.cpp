// Compiles only when BOSS_FEATURES_DIR points at
// src/boss/include/boss/test-fixtures. Proves the real Fill.cpp dispatch
// points for the solid_fill_policy registry route a generator-discovered
// feature through a real, processed Print, and that force_ensuring wins
// over preferred_pattern regardless of the registry's alphabetical fold
// order.
#include <catch2/catch_test_macros.hpp>

#include "boss/test-fixtures/solid-fill-policy-fixture/SolidFillPolicyFixtureFeature.hpp"
#include "libslic3r/Fill/Fill.hpp"
#include "libslic3r/Layer.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Surface.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

TEST_CASE("force_ensuring wins over preferred_pattern when the fixture forces it",
          "[boss][fill][fixture]")
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Domain::Percentage{15.0});
    config.print.items.opt("boss_test_solid_fill_policy_active").set(true);
    config.print.items.opt("boss_test_solid_fill_policy_force").set(true);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    bool found_internal_solid = false;
    for (const Layer *layer : print.get_object(0)->layers())
        for (const SurfaceFill &fill : group_fills(*layer))
            if (fill.surface.surface_type == stInternalSolid) {
                found_internal_solid = true;
                REQUIRE(fill.params.pattern == Domain::InfillPattern::ipEnsuring);
                REQUIRE(fill.params.pattern != Domain::InfillPattern::ipConcentric);
            }

    REQUIRE(found_internal_solid);
}

TEST_CASE("The fixture is inert for unrelated tests by default (boss_test_solid_fill_policy_active off)",
          "[boss][fill][fixture]")
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Domain::Percentage{15.0});

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    bool found_internal_solid = false;
    for (const Layer *layer : print.get_object(0)->layers())
        for (const SurfaceFill &fill : group_fills(*layer))
            if (fill.surface.surface_type == stInternalSolid) {
                found_internal_solid = true;
                REQUIRE(fill.params.pattern == Domain::InfillPattern::ipEnsuring);
            }

    REQUIRE(found_internal_solid);
}

TEST_CASE("preferred_pattern reaches Fill.cpp when the fixture does not force Ensuring",
          "[boss][fill][fixture]")
{
    TestConfig config;
    config.print.items.opt("skirts").set(0);
    config.print.items.opt("perimeters").set(1);
    config.print.items.opt("fill_density").set(Domain::Percentage{15.0});
    config.print.items.opt("boss_test_solid_fill_policy_active").set(true);
    config.print.items.opt("boss_test_solid_fill_policy_force").set(false);

    Print print;
    Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, config);

    bool found_internal_solid = false;
    for (const Layer *layer : print.get_object(0)->layers())
        for (const SurfaceFill &fill : group_fills(*layer))
            if (fill.surface.surface_type == stInternalSolid) {
                found_internal_solid = true;
                REQUIRE(fill.params.pattern == Domain::InfillPattern::ipConcentric);
            }

    REQUIRE(found_internal_solid);
}
