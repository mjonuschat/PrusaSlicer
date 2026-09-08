#include <catch2/catch_test_macros.hpp>

#include "libslic3r/Layer.hpp"
#include "libslic3r/Print.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

SCENARIO("BOSS external-first-holes changes the Classic perimeter print order", "[boss][perimeter]")
{
    GIVEN("a hollow model with external perimeters first for contours enabled") {
        TestConfig config;
        config.print.items.opt("perimeter_generator").set(Domain::PerimeterGeneratorType::Classic);
        config.print.items.opt("perimeters").set(2);
        config.print.items.opt("external_perimeters_first").set(true);
        config.print.items.opt("external_perimeters_first_holes_min_size").set(0.0);

        TestConfig holes_first_config = config;
        holes_first_config.print.items.opt("external_perimeters_first_holes").set(true);

        TestConfig holes_last_config = config;
        holes_last_config.print.items.opt("external_perimeters_first_holes").set(false);

        THEN("the two runs slice successfully and produce different toolpaths") {
            const std::string holes_first_gcode =
                Slic3r::Test::slice({TestMesh::two_hollow_squares}, holes_first_config);
            const std::string holes_last_gcode =
                Slic3r::Test::slice({TestMesh::two_hollow_squares}, holes_last_config);

            REQUIRE_FALSE(holes_first_gcode.empty());
            REQUIRE_FALSE(holes_last_gcode.empty());
            REQUIRE(holes_first_gcode != holes_last_gcode);
        }
    }
}
