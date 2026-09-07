#include <catch2/catch_test_macros.hpp>

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "libslic3r/StepsInvalidation.hpp"

using namespace Slic3r;              // FDMPrintStep enumerators (psGCodeExport, psWipeTower)
using namespace Slic3r::SlicingSync; // Step, steps, propagate, merge_boss_invalidations

TEST_CASE("merge_boss_invalidations adds BOSS entries", "[boss][invalidation]")
{
    std::map<std::string, std::vector<Step>> base{{"perimeter_speed", steps({propagate(psGCodeExport)})}};
    std::map<std::string, std::vector<Step>> boss{{"filament_max_speed", steps({propagate(psWipeTower)})}};
    const auto merged = merge_boss_invalidations(base, boss);
    REQUIRE(merged.count("perimeter_speed") == 1);
    REQUIRE(merged.count("filament_max_speed") == 1);
}

TEST_CASE("merge_boss_invalidations rejects a collision with an upstream key", "[boss][invalidation]")
{
    std::map<std::string, std::vector<Step>> base{{"perimeter_speed", steps({propagate(psGCodeExport)})}};
    std::map<std::string, std::vector<Step>> boss{{"perimeter_speed", steps({propagate(psWipeTower)})}};
    REQUIRE_THROWS_AS(merge_boss_invalidations(base, boss), std::logic_error);
}
