#include <catch2/catch_test_macros.hpp>

#include "boss/foundation/BossSolidFillPolicyRegistry.hpp"
#include "libslic3r/ConfigViews.hpp"
#include "libslic3r/boss/surface/SolidFillPolicyContext.hpp"

namespace {

using Slic3r::ExPolygons;
using Slic3r::PrintRegionConfigView;
using Slic3r::Boss::SolidFillPolicyContext;
namespace Domain = Slic3r::Domain;

ExPolygons make_expolygons() { return {}; }

const PrintRegionConfigView &default_region_config()
{
    static const PrintRegionConfigView config;
    return config;
}

struct SilentFeature {
    static constexpr int id = 1;
};

struct SkipTrueFeature {
    static constexpr int id = 2;
    static bool skip_narrow_top_bottom(const SolidFillPolicyContext &) { return true; }
};

struct ForceTrueFeature {
    static constexpr int id = 3;
    static bool force_ensuring(const SolidFillPolicyContext &) { return true; }
};

struct PreferredConcentricFeature {
    static constexpr int id = 4;
    static std::optional<Domain::InfillPattern> preferred_pattern(const SolidFillPolicyContext &)
    {
        return Domain::InfillPattern::ipConcentric;
    }
};

} // namespace

TEST_CASE("BossSolidFillPolicyRegistry with zero features returns the empty defaults", "[boss][fill]")
{
    using Registry = Slic3r::Boss::BossSolidFillPolicyRegistry<>;
    ExPolygons expolygons = make_expolygons();
    SolidFillPolicyContext ctx{default_region_config(), expolygons, coord_t(0)};

    REQUIRE(Registry::skip_narrow_top_bottom(ctx) == false);
    REQUIRE(Registry::force_ensuring(ctx) == false);
    REQUIRE(Registry::preferred_pattern(ctx) == std::nullopt);
}

TEST_CASE("BossSolidFillPolicyRegistry OR-folds skip_narrow_top_bottom regardless of order", "[boss][fill]")
{
    ExPolygons expolygons = make_expolygons();
    SolidFillPolicyContext ctx{default_region_config(), expolygons, coord_t(0)};

    using ForwardOrder = Slic3r::Boss::BossSolidFillPolicyRegistry<SilentFeature, SkipTrueFeature>;
    using ReverseOrder = Slic3r::Boss::BossSolidFillPolicyRegistry<SkipTrueFeature, SilentFeature>;

    REQUIRE(ForwardOrder::skip_narrow_top_bottom(ctx) == true);
    REQUIRE(ReverseOrder::skip_narrow_top_bottom(ctx) == true);
}

TEST_CASE("BossSolidFillPolicyRegistry OR-folds force_ensuring across two contributing features", "[boss][fill]")
{
    ExPolygons expolygons = make_expolygons();
    SolidFillPolicyContext ctx{default_region_config(), expolygons, coord_t(0)};

    using Forward = Slic3r::Boss::BossSolidFillPolicyRegistry<ForceTrueFeature, SilentFeature>;
    using Reverse = Slic3r::Boss::BossSolidFillPolicyRegistry<SilentFeature, ForceTrueFeature>;

    REQUIRE(Forward::force_ensuring(ctx) == true);
    REQUIRE(Reverse::force_ensuring(ctx) == true);
    REQUIRE(Forward::skip_narrow_top_bottom(ctx) == false);
}

TEST_CASE("BossSolidFillPolicyRegistry preferred_pattern surfaces a contributing feature's suggestion", "[boss][fill]")
{
    using Registry = Slic3r::Boss::BossSolidFillPolicyRegistry<SilentFeature, PreferredConcentricFeature>;
    ExPolygons expolygons = make_expolygons();
    SolidFillPolicyContext ctx{default_region_config(), expolygons, coord_t(0)};

    auto pattern = Registry::preferred_pattern(ctx);
    REQUIRE(pattern.has_value());
    REQUIRE(*pattern == Domain::InfillPattern::ipConcentric);
}

TEST_CASE("BossSolidFillPolicyRegistry force_ensuring and preferred_pattern are independent folds", "[boss][fill]")
{
    using Registry = Slic3r::Boss::BossSolidFillPolicyRegistry<ForceTrueFeature, PreferredConcentricFeature>;
    ExPolygons expolygons = make_expolygons();
    SolidFillPolicyContext ctx{default_region_config(), expolygons, coord_t(0)};

    REQUIRE(Registry::force_ensuring(ctx) == true);
    auto pattern = Registry::preferred_pattern(ctx);
    REQUIRE(pattern.has_value());
    REQUIRE(*pattern == Domain::InfillPattern::ipConcentric);
}
