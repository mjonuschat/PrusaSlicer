#include <catch2/catch_test_macros.hpp>

#include "test_data.hpp"

using namespace Slic3r;

static bool has_toolchange_command(const std::string& gcode, unsigned int extruder_id)
{
    return gcode.find("\nT" + std::to_string(extruder_id) + "\n") != std::string::npos;
}

SCENARIO("BOSS autoemit_toolchange_commands suppresses the automatic T-command", "[boss][toolchange]")
{
    GIVEN("custom toolchange G-code with no T-command of its own") {
        auto make_config = [](bool autoemit) {
            Test::TestConfig config{2, 0.4};
            config.print.items.opt("perimeter_extruder").set(1);
            config.print.items.opt("infill_extruder").set(2);
            config.printer.items.opt("toolchange_gcode").set(std::string("; custom toolchange, no T-command"));
            config.printer.items.opt("autoemit_toolchange_commands").set(autoemit);
            return config;
        };

        WHEN("autoemit_toolchange_commands is enabled (default)") {
            std::string gcode = Slic3r::Test::slice({ Slic3r::Test::TestMesh::cube_20x20x20 }, make_config(true));
            THEN("the automatic T-command is still appended") {
                REQUIRE(has_toolchange_command(gcode, 1));
            }
        }

        WHEN("autoemit_toolchange_commands is disabled") {
            std::string gcode = Slic3r::Test::slice({ Slic3r::Test::TestMesh::cube_20x20x20 }, make_config(false));
            THEN("no T-command is emitted at all") {
                REQUIRE(! has_toolchange_command(gcode, 1));
            }
        }
    }
}
