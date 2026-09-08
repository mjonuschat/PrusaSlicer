#include <catch2/catch_test_macros.hpp>

#include "boss/foundation/BossPerimeterPolicyRegistry.hpp"
#include "libslic3r/boss/perimeter/PerimeterPolicyContext.hpp"

namespace {

struct LoopBumpFeature {
    static constexpr int id = 1;
    static int adjust_loop_count(int loop_count, const Slic3r::Boss::PerimeterPolicyContext &ctx)
    {
        return ctx.fill_density > 0.0 ? loop_count + 1 : loop_count;
    }
};

struct HoleOrderFeature {
    static constexpr int id = 2;
    static void apply_ordering(Slic3r::Boss::OrderingPolicy &policy, const Slic3r::Boss::PerimeterPolicyContext &)
    {
        policy.holes_external_first = true;
    }
};

} // namespace

TEST_CASE("BossPerimeterPolicyRegistry folds loop-count adjustments", "[boss][perimeter]")
{
    using Registry = Slic3r::Boss::BossPerimeterPolicyRegistry<LoopBumpFeature>;
    Slic3r::Boss::PerimeterPolicyContext ctx;
    ctx.fill_density = 15.0;
    REQUIRE(Registry::adjust_loop_count(2, ctx) == 3);
}

TEST_CASE("BossPerimeterPolicyRegistry folds ordering-policy contributions", "[boss][perimeter]")
{
    using Registry = Slic3r::Boss::BossPerimeterPolicyRegistry<HoleOrderFeature>;
    Slic3r::Boss::PerimeterPolicyContext ctx;
    Slic3r::Boss::OrderingPolicy policy;
    Registry::apply_ordering(policy, ctx);
    REQUIRE(policy.holes_external_first == true);
}

TEST_CASE("BossPerimeterPolicyRegistry with zero features is a no-op", "[boss][perimeter]")
{
    using Registry = Slic3r::Boss::BossPerimeterPolicyRegistry<>;
    Slic3r::Boss::PerimeterPolicyContext ctx;
    Slic3r::Boss::OrderingPolicy policy;
    Registry::apply_ordering(policy, ctx);
    REQUIRE(Registry::adjust_loop_count(2, ctx) == 2);
    REQUIRE(policy.holes_external_first == false);
}
