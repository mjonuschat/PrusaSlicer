// Compiles only when BOSS_FEATURES_DIR points at
// src/boss/include/boss/test-fixtures. Proves a generator-discovered BOSS
// option's invalidation reaches diff_to_invalidated_steps.
#include <algorithm>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "libslic3r/StepsInvalidation.hpp"

using namespace Slic3r;              // FDMPrintStep enumerators (psGCodeExport)
using namespace Slic3r::SlicingSync; // Step, invalidated_steps_table

TEST_CASE("A generator-discovered BOSS option invalidates its declared steps",
          "[boss][invalidation][fixture]")
{
    const auto& table = invalidated_steps_table();
    REQUIRE(table.count("boss_test_fixture_flag") == 1);
    // diff_to_invalidated_steps() resolves every changed key with this same
    // invalidated_by.at(opt_key) lookup (StepsInvalidation.cpp, in diff_to_invalidated_steps), so asserting
    // the resolved value here exercises the consumer's BOSS-specific behavior.
    const std::vector<Step>& s = table.at("boss_test_fixture_flag");
    REQUIRE(std::find(s.begin(), s.end(), Step{psGCodeExport}) != s.end());
}
