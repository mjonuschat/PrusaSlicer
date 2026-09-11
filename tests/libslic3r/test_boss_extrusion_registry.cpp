#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "boss/foundation/BossExtrusionRegistry.hpp"

using namespace Slic3r::Boss;
using Slic3r::ExtrusionRole;

namespace {

struct FixtureDynamicsFeature {
    static constexpr int id = 900001;
    static std::optional<MotionDynamics> before_extrusion(const ExtrusionContext &ctx)
    {
        if (ctx.extruder_id == 0)
            return MotionDynamics{1000, 0.5, 5000};
        return std::nullopt;
    }
};

struct FixtureFlowFeature {
    static constexpr int id = 900002;
    static double modify_flow(double dE, const ExtrusionContext &) { return dE * 0.5; }
};

struct FixtureRetractFeature {
    static constexpr int id = 900003;
    static double modify_retract(double lift, const ExtrusionContext &) { return 0.0; }
};

struct FixtureNoopFeature {
    static constexpr int id = 900004;
};

struct FixtureFlowHalveFeature {
    static constexpr int id = 900005;
    static double modify_flow(double dE, const ExtrusionContext &) { return dE * 0.5; }
};

struct FixtureFlowAddOneFeature {
    static constexpr int id = 900006;
    static double modify_flow(double dE, const ExtrusionContext &) { return dE + 1.0; }
};

struct FixtureRetractDoubleFeature {
    static constexpr int id = 900007;
    static double modify_retract(double lift, const ExtrusionContext &) { return lift * 2.0; }
};

struct FixtureRetractSubtractFeature {
    static constexpr int id = 900008;
    static double modify_retract(double lift, const ExtrusionContext &) { return lift - 0.1; }
};

} // namespace

TEST_CASE("BossExtrusionRegistry folds before_extrusion, modify_flow, modify_retract", "[boss][extrusion]")
{
    using Registry = BossExtrusionRegistry<FixtureDynamicsFeature, FixtureFlowFeature, FixtureRetractFeature, FixtureNoopFeature>;

    ExtrusionContext ctx{ExtrusionRole::InternalInfill, false, false, 0};

    SECTION("before_extrusion returns the contributing feature's value")
    {
        auto dynamics = Registry::before_extrusion(ctx);
        REQUIRE(dynamics.has_value());
        CHECK(dynamics->acceleration == 1000);
        CHECK(dynamics->minimum_cruise_ratio == 0.5);
        CHECK(dynamics->jerk == 5000);
    }

    SECTION("before_extrusion returns nullopt when no feature is decisive")
    {
        ExtrusionContext other_extruder{ExtrusionRole::InternalInfill, false, false, 1};
        auto dynamics = Registry::before_extrusion(other_extruder);
        CHECK_FALSE(dynamics.has_value());
    }

    SECTION("modify_flow pipes through the contributing feature")
    {
        CHECK(Registry::modify_flow(1.0, ctx) == 0.5);
    }

    SECTION("modify_retract pipes through the contributing feature")
    {
        CHECK(Registry::modify_retract(0.4, ctx) == 0.0);
    }

    SECTION("empty registry is a no-op pass-through")
    {
        using Empty = BossExtrusionRegistry<>;
        CHECK_FALSE(Empty::before_extrusion(ctx).has_value());
        CHECK(Empty::modify_flow(1.0, ctx) == 1.0);
        CHECK(Empty::modify_retract(0.4, ctx) == 0.4);
    }
}

TEST_CASE("BossExtrusionRegistry pipeline-folds modify_flow across two contributing features", "[boss][extrusion]")
{
    ExtrusionContext ctx{ExtrusionRole::InternalInfill, false, false, 0};

    using Forward = BossExtrusionRegistry<FixtureFlowHalveFeature, FixtureFlowAddOneFeature>;
    using Reverse = BossExtrusionRegistry<FixtureFlowAddOneFeature, FixtureFlowHalveFeature>;

    CHECK(Forward::modify_flow(1.0, ctx) == 1.5);
    CHECK(Reverse::modify_flow(1.0, ctx) == 1.0);
}

TEST_CASE("BossExtrusionRegistry pipeline-folds modify_retract across two contributing features", "[boss][extrusion]")
{
    ExtrusionContext ctx{ExtrusionRole::InternalInfill, false, false, 0};

    using Forward = BossExtrusionRegistry<FixtureRetractDoubleFeature, FixtureRetractSubtractFeature>;
    using Reverse = BossExtrusionRegistry<FixtureRetractSubtractFeature, FixtureRetractDoubleFeature>;

    CHECK(Forward::modify_retract(0.4, ctx) == Catch::Approx(0.7));
    CHECK(Reverse::modify_retract(0.4, ctx) == Catch::Approx(0.6));
}

TEST_CASE("ExtrusionContext carries path_length, defaulting to zero", "[boss][extrusion]")
{
    ExtrusionContext ctx{ExtrusionRole::InternalInfill, false, false, 0};
    CHECK(ctx.path_length == 0.0);

    ExtrusionContext with_length{ExtrusionRole::InternalInfill, false, false, 0};
    with_length.path_length = 12.5;
    CHECK(with_length.path_length == 12.5);
}
