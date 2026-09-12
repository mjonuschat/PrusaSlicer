#include <catch2/catch_test_macros.hpp>

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

SCENARIO("BOSS preheat_toolchangers forces M104.1 preheat on a non-XL printer", "[boss][toolchange]")
{
    GIVEN("a multi-extruder print with preheat_toolchangers enabled") {
        TestConfig config{2, 0.4};
        config.print.items.opt("perimeter_extruder").set(1);
        config.print.items.opt("infill_extruder").set(2);
        config.print.items.opt("preheat_toolchangers").set(true);

        std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, config);

        THEN("the emitted G-code contains an M104.1 preheat command") {
            REQUIRE(gcode.find("M104.1") != std::string::npos);
        }
    }
}
