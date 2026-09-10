#include "boss/test-fixtures/solid-fill-policy-fixture/SolidFillPolicyFixtureFeature.hpp"

#include "libslic3r/ConfigViews.hpp"
#include "libslic3r/boss/surface/SolidFillPolicyContext.hpp"

namespace Slic3r::Boss::Test {

bool SolidFillPolicyFixtureFeature::force_ensuring(const Slic3r::Boss::SolidFillPolicyContext &ctx)
{
    return ctx.region_config.get<bool>("boss_test_solid_fill_policy_force");
}

std::optional<Domain::InfillPattern> SolidFillPolicyFixtureFeature::preferred_pattern(const Slic3r::Boss::SolidFillPolicyContext &ctx)
{
    if (! ctx.region_config.get<bool>("boss_test_solid_fill_policy_active"))
        return std::nullopt;
    return Domain::InfillPattern::ipConcentric;
}

} // namespace Slic3r::Boss::Test
