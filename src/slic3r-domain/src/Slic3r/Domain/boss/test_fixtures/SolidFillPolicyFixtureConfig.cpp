// src/slic3r-domain/src/Slic3r/Domain/boss/test_fixtures/SolidFillPolicyFixtureConfig.cpp
#include "boss/test-fixtures/solid-fill-policy-fixture/SolidFillPolicyFixtureFeature.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"

namespace Slic3r::Boss::Test {

void SolidFillPolicyFixtureFeature::register_config(Domain::ConfigDefinitions& defs)
{
    using namespace Slic3r::Domain;

    // Both options default to false so this fixture is inert for every
    // OTHER test built against the fixture composition -- an always-on
    // preferred_pattern() would silently change unrelated tests' internal-
    // solid pattern.
    ConfigItemDef* active = defs.add("boss_test_solid_fill_policy_active", typeid(bool));
    active->location = FDMConfigLocation::Print;
    active->category = ConfigItemDef::Category::Hidden;
    active->init_fn  = init_with(false);

    ConfigItemDef* def = defs.add("boss_test_solid_fill_policy_force", typeid(bool));
    def->location = FDMConfigLocation::Print;
    def->category = ConfigItemDef::Category::Hidden;
    def->init_fn  = init_with(false);
}

} // namespace Slic3r::Boss::Test
