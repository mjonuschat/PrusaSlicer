#include <catch2/catch_test_macros.hpp>

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

static TestConfig make_two_tool_config()
{
    TestConfig config{2, 0.4};
    config.print.items.opt("perimeter_extruder").set(1);
    config.print.items.opt("solid_infill_extruder").set(2);
    config.print.items.opt("ooze_prevention").set(true);
    config.print.items.opt("preheat_time").set(1.);
    config.print.items.opt("skirts").set(0);
    config.printer.items.opt("start_gcode").set(std::string{""});
    config.filament.at(0).items.opt("temperature").set(200);
    config.filament.at(1).items.opt("temperature").set(205);
    config.filament.at(0).items.opt("first_layer_temperature").set(210);
    config.filament.at(1).items.opt("first_layer_temperature").set(215);
    return config;
}

static std::string line_containing(const std::string& gcode, const std::string& needle)
{
    const size_t pos = gcode.find(needle);
    if (pos == std::string::npos)
        return {};
    const size_t begin = gcode.rfind('\n', pos);
    const size_t start = begin == std::string::npos ? 0 : begin + 1;
    const size_t end = gcode.find('\n', pos);
    return gcode.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

SCENARIO("BOSS preheats tool changes on any multi-tool printer", "[boss][toolchange]")
{
    GIVEN("a two-tool print with ooze prevention on and no tool-preheat printer feature") {
        TestConfig config = make_two_tool_config();

        std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, config);

        THEN("the next tool is preheated to its first layer temperature") {
            REQUIRE(line_containing(gcode, "preheat T1").find("M104 S215 T1") != std::string::npos);
        }
        THEN("no Prusa XL specific M104.1 command is emitted") {
            REQUIRE(gcode.find("M104.1") == std::string::npos);
        }
        THEN("the ooze prevention cooldown commands stay in the G-code") {
            REQUIRE(gcode.find("cooldown") != std::string::npos);
        }
    }

    GIVEN("the same print with the preheat time set to zero") {
        TestConfig config = make_two_tool_config();
        config.print.items.opt("preheat_time").set(0.);

        std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, config);

        THEN("no preheat command is emitted") {
            REQUIRE(gcode.find("preheat T") == std::string::npos);
        }
    }

    GIVEN("the same print with ooze prevention off") {
        TestConfig config = make_two_tool_config();
        config.print.items.opt("ooze_prevention").set(false);

        std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, config);

        THEN("the next tool is still preheated") {
            REQUIRE(gcode.find("preheat T1") != std::string::npos);
        }
    }

    GIVEN("a single extruder multi material printer") {
        TestConfig config = make_two_tool_config();
        config.printer.items.opt("single_extruder_multi_material").set(true);

        std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, config);

        THEN("no preheat command is emitted") {
            REQUIRE(gcode.find("preheat T") == std::string::npos);
        }
    }
}
