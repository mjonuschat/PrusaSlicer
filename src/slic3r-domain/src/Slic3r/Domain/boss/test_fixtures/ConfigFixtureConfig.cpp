// src/slic3r-domain/src/Slic3r/Domain/boss/test_fixtures/ConfigFixtureConfig.cpp
#include "boss/test-fixtures/config-fixture/ConfigFixtureFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss::Test {

void ConfigFixtureFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    // Category::Hidden: never shown even if a Settings GUI is built later.
    // This option has no real-world meaning -- it exists only so a test
    // can assert it was actually registered by the real
    // fdm_config_init_fn() call site, not simulated.
    ConfigItemDef* def = defs.add("boss_test_fixture_flag", typeid(bool));
    def->location = FDMConfigLocation::Printer;
    def->category = ConfigItemDef::Category::Hidden;
    def->init_fn  = init_with(false);

    // Category::Hidden: never shown even if a Settings GUI is built later.
    // This option has no real-world meaning -- it exists only so a test
    // can assert it was actually registered by the real
    // fdm_config_init_fn() call site, not simulated.
    def = defs.add("boss_test_fixture_purge", typeid(bool));
    def->location = FDMConfigLocation::Printer;
    def->category = ConfigItemDef::Category::Hidden;
    def->init_fn  = init_with(false);
}

} // namespace Slic3r::Boss::Test
