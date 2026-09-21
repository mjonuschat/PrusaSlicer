#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

SCENARIO("BOSS wipe tower max purge speed caps emitted feedrate during purge", "[boss][toolchange]")
{
    GIVEN("a two-extruder print, forced to tool-change, with a low wipe tower max purge speed") {
        TestConfig config{2, 0.4};
        // Perimeters and infill use different tools. This forces a toolchange
        // and a wipe tower purge. Without a toolchange, toolchange_Wipe() never
        // runs and the feedrate check below passes for the wrong reason.
        config.print.items.opt("perimeter_extruder").set(1);
        config.print.items.opt("infill_extruder").set(2);
        config.printer.items.opt("single_extruder_multi_material").set(true);
        config.print.items.opt("wipe_tower").set(true);
        config.print.items.opt("wipe_tower_max_purge_speed").set(20.0);
        config.print.items.opt("retract_length").set(0.0);
        config.printer.items.opt("use_relative_e_distances").set(true);
        config.print.items.opt("support_material_extruder").set(0);
        config.print.items.opt("support_material_interface_extruder").set(0);

        std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, config);

        THEN("no extrusion move inside the wipe-tower purge block exceeds F1200 (20mm/s * 60)") {
            // The clamp applies only to toolchange_Wipe()'s target speed, not
            // to the object's own infill and perimeter speeds. Those default
            // well above 20mm/s, so the check scans only the wipe block. The
            // block starts at "; CP TOOLCHANGE WIPE" and ends at the next
            // "; CP " comment or end of file.
            std::istringstream stream(gcode);
            std::string line;
            bool in_wipe_block = false;
            bool saw_wipe_block = false;
            while (std::getline(stream, line)) {
                if (line.find("; CP TOOLCHANGE WIPE") != std::string::npos) {
                    in_wipe_block = true;
                    saw_wipe_block = true;
                    continue;
                }
                if (in_wipe_block && line.rfind("; CP ", 0) == 0) {
                    in_wipe_block = false;
                    continue;
                }
                if (!in_wipe_block) continue;
                if (line.empty() || line[0] != 'G') continue;
                if (line.find('E') == std::string::npos) continue;
                auto pos = line.find('F');
                if (pos == std::string::npos) continue;
                double feedrate = std::stod(line.substr(pos + 1));
                INFO("offending line: " << line);
                REQUIRE(feedrate <= 1200.0 + 1e-6);
            }
            REQUIRE(saw_wipe_block);
        }
    }
}
