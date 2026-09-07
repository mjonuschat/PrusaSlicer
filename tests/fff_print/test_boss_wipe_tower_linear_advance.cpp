#include <catch2/catch_test_macros.hpp>

#include <string>

#include "Slic3r/Domain/GCodeFlavor.hpp"
#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;
using namespace std::string_literals;

SCENARIO("BOSS wipe_tower_disable_linear_advance forces suppression at both wipe points", "[boss][toolchange]")
{
    GIVEN("a two-tool print with and without linear advance suppression forced on") {
        auto count_m900_k0 = [](const std::string& gcode) {
            size_t count = 0;
            size_t pos = 0;
            while ((pos = gcode.find("M900 K0", pos)) != std::string::npos) { ++count; pos += 7; }
            return count;
        };

        auto make_config = [](bool disable_linear_advance) {
            TestConfig config{2, 0.4};
            config.printer.items.opt("gcode_flavor").set(Domain::GCodeFlavor::gcfMarlinFirmware);
            config.print.items.opt("perimeter_extruder").set(1);
            config.print.items.opt("infill_extruder").set(2);
            config.printer.items.opt("single_extruder_multi_material").set(true);
            config.printer.items.opt("use_relative_e_distances").set(true);
            config.printer.items.opt("layer_gcode").set("G92 E0"s);
            config.print.items.opt("support_material_extruder").set(0);
            config.print.items.opt("support_material_interface_extruder").set(0);
            config.print.items.opt("wipe_tower").set(true);
            config.printer.items.opt("enable_pressure_advance_during_ramming").set(true);
            config.print.items.opt("wipe_tower_disable_linear_advance").set(disable_linear_advance);
            return config;
        };

        std::string gcode_native = Slic3r::Test::slice({ Slic3r::Test::TestMesh::cube_20x20x20 }, make_config(false));
        std::string gcode_forced = Slic3r::Test::slice({ Slic3r::Test::TestMesh::cube_20x20x20 }, make_config(true));

        THEN("forcing suppression at both wipe points strictly increases the linear-advance-disable count") {
            REQUIRE(count_m900_k0(gcode_native) < count_m900_k0(gcode_forced));
        }
    }
}
