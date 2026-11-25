#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

SCENARIO("BOSS per-object extrusion multiplier scales emitted flow", "[boss][flow]")
{
    GIVEN("a cube sliced at extrusion multiplier 1.0 and again at 1.2") {
        TestConfig baseline;
        baseline.print.items.opt("skirts").set(0);
        baseline.print.items.opt("print_extrusion_multiplier").set(1.0);
        std::string gcode_baseline = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, baseline);

        TestConfig scaled;
        scaled.print.items.opt("skirts").set(0);
        scaled.print.items.opt("print_extrusion_multiplier").set(1.2);
        std::string gcode_scaled = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, scaled);

        THEN("the scaled G-code contains larger E values on comparable extrusion moves") {
            auto total_e = [](const std::string& gcode) {
                double sum = 0;
                std::istringstream stream(gcode);
                std::string line;
                while (std::getline(stream, line)) {
                    auto pos = line.find('E');
                    if (pos != std::string::npos && line[0] == 'G')
                        sum += std::stod(line.substr(pos + 1));
                }
                return sum;
            };
            REQUIRE(total_e(gcode_scaled) > total_e(gcode_baseline) * 1.15);
        }
    }
}
