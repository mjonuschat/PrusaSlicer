#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <variant>

#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"

TEST_CASE("BOSS expanded filament types are present", "[boss][config]")
{
    const Slic3r::Domain::ConfigDefinitions& defs = Slic3r::Domain::get_defs_fdm();
    const auto it = std::find_if(
        defs.defs().begin(), defs.defs().end(),
        [](const Slic3r::Domain::ConfigItemDef& def) { return def.name == "filament_type"; }
    );
    REQUIRE(it != defs.defs().end());

    auto has_choice = [&](const std::string& expected) {
        return std::any_of(
            it->choices.begin(), it->choices.end(),
            [&](const auto& choice) {
                const auto* value = std::get_if<std::string>(&choice.first);
                return value != nullptr && *value == expected;
            }
        );
    };

    for (const std::string& expected : {"ABS-CF", "PETG-CF", "PC-CF", "PPS", "TPU", "PHA"}) {
        INFO("missing filament type: " << expected);
        REQUIRE(has_choice(expected));
    }

    for (const std::string& native : {"PLA", "PETG", "ABS", "ASA"}) {
        INFO("dropped native filament type: " << native);
        REQUIRE(has_choice(native));
    }
}
