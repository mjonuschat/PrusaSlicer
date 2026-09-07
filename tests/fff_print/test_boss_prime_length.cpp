#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <vector>

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

namespace {

bool is_standalone_prime_move(const std::string& line, double e_value)
{
    return line.find('X') == std::string::npos && line.find('Y') == std::string::npos
        && e_value > 4.0 && e_value < 6.0;
}

std::vector<std::string> find_prime_moves(const std::string& gcode)
{
    std::vector<std::string> found;
    std::istringstream stream(gcode);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] != 'G') continue;
        auto e_pos = line.find('E');
        if (e_pos == std::string::npos) continue;
        double e_value = std::stod(line.substr(e_pos + 1));
        if (is_standalone_prime_move(line, e_value))
            found.push_back(line);
    }
    return found;
}

} // namespace

SCENARIO("BOSS prime_length_at_start primes the extruder before the first perimeter", "[boss][toolchange]")
{
    GIVEN("a cube with a nonzero prime length for the first extruder and no confounding retraction") {
        TestConfig config;
        config.print.items.opt("retract_length").set(0.0);
        config.filament[0].items.opt("prime_length_at_start").set(5.0);

        std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, config);

        THEN("an E-axis move of approximately the prime length appears before the first perimeter extrusion") {
            std::vector<std::string> prime_moves = find_prime_moves(gcode);
            REQUIRE(! prime_moves.empty());
        }

        THEN("the prime move carries an explicit feedrate") {
            std::vector<std::string> prime_moves = find_prime_moves(gcode);
            REQUIRE(! prime_moves.empty());
            REQUIRE(prime_moves.front().find('F') != std::string::npos);
        }
    }

    GIVEN("a multi-layer cube with ramping travel lift enabled, which re-enters the priming code "
          "path at every layer's first travel move") {
        TestConfig config;
        config.print.items.opt("retract_length").set(0.0);
        config.print.items.opt("travel_ramping_lift").set(true);
        config.filament[0].items.opt("prime_length_at_start").set(5.0);

        std::string gcode = Slic3r::Test::slice({ TestMesh::cube_20x20x20 }, config);

        THEN("the prime move appears exactly once in the whole print, not once per layer") {
            std::vector<std::string> prime_moves = find_prime_moves(gcode);
            REQUIRE(prime_moves.size() == 1);
        }
    }
}
