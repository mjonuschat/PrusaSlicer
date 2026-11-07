#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;
using Domain::Percentage;

SCENARIO(
    "BOSS 3D Honeycomb bridging uses the generic anchor-based angle detector", "[boss][infill]"
)
{
    GIVEN("an overhang model sliced with combined 3D Honeycomb sparse infill") {
        TestConfig config;
        config.print.items.opt("fill_pattern").set(std::string("3dhoneycomb"));
        config.print.items.opt("fill_density").set(Percentage{15.0});
        config.print.items.opt("infill_every_layers").set(3);

        std::string gcode = Slic3r::Test::slice({ TestMesh::overhang }, config);

        THEN("the print slices to completion and emits bridge extrusion moves") {
            // determine_bridging_angle is a private lambda inside
            // PrintObject::bridge_over_infill() -- it cannot be unit-tested
            // directly.
            REQUIRE_FALSE(gcode.empty());

            const auto config_footer_pos = gcode.find("; prusaslicer_config = begin");
            const std::string body = gcode.substr(
                0, config_footer_pos == std::string::npos ? gcode.size() : config_footer_pos
            );

            std::istringstream stream(body);
            std::string line;
            bool saw_bridge_infill = false;
            while (std::getline(stream, line)) {
                if (line.find(";TYPE:Bridge infill") != std::string::npos) {
                    saw_bridge_infill = true;
                    break;
                }
            }
            REQUIRE(saw_bridge_infill);
        }
    }
}
