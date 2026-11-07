// Proves the structured-fuzzy-skin feature's 4 config options reach the
// real fdm_config_init_fn() call site through the generator-discovered
// manifest, not a synthetic registration.
#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefsFDM.hpp"

using Slic3r::Domain::ConfigItemDef;
using Slic3r::Domain::ConfigDefinitions;
using Slic3r::Domain::get_defs_fdm;

namespace {

bool has_option(const ConfigDefinitions &defs, const std::string &name)
{
    return std::any_of(defs.defs().begin(), defs.defs().end(),
                        [&name](const ConfigItemDef &def) { return def.name == name; });
}

} // namespace

TEST_CASE("Structured fuzzy skin config options are registered", "[boss][config]")
{
    const ConfigDefinitions &defs = get_defs_fdm();

    CHECK(has_option(defs, "fuzzy_skin_noise_type"));
    CHECK(has_option(defs, "fuzzy_skin_feature_size"));
    CHECK(has_option(defs, "fuzzy_skin_octaves"));
    CHECK(has_option(defs, "fuzzy_skin_persistence"));
}
