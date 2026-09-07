#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

SCENARIO("BOSS wipe_tower_disable_filament_ramming removes ramming moves", "[boss][toolchange]")
{
    GIVEN("a two-tool print with and without ramming disabled") {
        auto count_unload_block_e_moves = [](const std::string& gcode) {
            std::istringstream stream(gcode);
            std::string line;
            bool in_unload_block = false;
            size_t count = 0;
            while (std::getline(stream, line)) {
                if (line.find("; CP TOOLCHANGE UNLOAD") != std::string::npos) { in_unload_block = true; continue; }
                if (in_unload_block && line.rfind("; CP ", 0) == 0) { in_unload_block = false; continue; }
                if (in_unload_block && !line.empty() && line[0] == 'G' && line.find('E') != std::string::npos) ++count;
            }
            return count;
        };

        auto make_config = [](bool disable_ramming) {
            TestConfig config{2, 0.4};
            config.print.items.opt("perimeter_extruder").set(1);
            config.print.items.opt("infill_extruder").set(2);
            config.printer.items.opt("single_extruder_multi_material").set(true);
            config.printer.items.opt("use_relative_e_distances").set(true);
            config.print.items.opt("support_material_extruder").set(0);
            config.print.items.opt("support_material_interface_extruder").set(0);
            config.print.items.opt("wipe_tower").set(true);
            config.print.items.opt("wipe_tower_disable_filament_ramming").set(disable_ramming);
            return config;
        };

        std::string gcode_with_ramming = Slic3r::Test::slice({ Slic3r::Test::TestMesh::cube_20x20x20 }, make_config(false));
        std::string gcode_without_ramming = Slic3r::Test::slice({ Slic3r::Test::TestMesh::cube_20x20x20 }, make_config(true));

        THEN("disabling ramming strictly reduces the unload block's extrusion move count") {
            REQUIRE(count_unload_block_e_moves(gcode_without_ramming) < count_unload_block_e_moves(gcode_with_ramming));
        }
    }
}
