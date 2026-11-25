#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

SCENARIO("BOSS filament max speed caps emitted feedrate", "[boss][motion]")
{
    GIVEN("a cube with a high infill speed but a low filament max speed") {
        TestConfig config;
        config.print.items.opt("skirts").set(0);
        config.print.items.opt("infill_speed").set(300.0);
        config.filament[0].items.opt("filament_max_speed").set(20.0);
        // cap_speed() only governs extrusion path speed -- retraction moves
        // are a separate speed setting (retract_speed, Print-level, overridden
        // per Tool/Filament) that legitimately exceeds this cap and are not
        // what this feature governs. Disable retraction entirely so every
        // line carrying an E value is an actual extrusion move.
        config.print.items.opt("retract_length").set(0.0);

        std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, config);

        THEN("no G1 extrusion move exceeds F1200 (20mm/s * 60)") {
            std::istringstream stream(gcode);
            std::string line;
            // The G-code writer only emits F when the feedrate changes, so an
            // extrusion move's speed comes from the most recent F seen, not
            // necessarily from its own line.
            double current_feedrate = 0.0;
            bool checked_any = false;
            while (std::getline(stream, line)) {
                if (line.compare(0, 3, "G1 ") != 0) continue;
                auto fpos = line.find('F');
                if (fpos != std::string::npos) {
                    current_feedrate = std::stod(line.substr(fpos + 1));
                }
                if (line.find('E') == std::string::npos) continue;
                checked_any = true;
                INFO("offending line: " << line);
                REQUIRE(current_feedrate <= 1200.0 + 1e-6);
            }
            REQUIRE(checked_any);
        }
    }
}
