// tests/libslic3r/test_boss_fill_dispatch.cpp
#include <catch2/catch_test_macros.hpp>

#include "boss/foundation/BossFillRegistry.hpp"
#include "libslic3r/Fill/FillRectilinear.hpp"

namespace {
// Local, test-only trait shapes -- not registered BOSS features, never
// touched by the generator or any manifest. Instantiating
// BossFillRegistry directly with two fake traits -- one true, one false
// -- tests the generic per-feature dispatch mechanism itself,
// independent of which real features (if any) are compiled in.
struct FakeFillType : Slic3r::FillRectilinear {
    Fill* clone() const override { return new FakeFillType(*this); }
};

struct FakeAnchoringTrueFeature {
    static constexpr int id = 900001;
    static constexpr std::string_view key = "fake-anchoring-true";
    static constexpr std::string_view label = "FakeAnchoringTrue";
    static constexpr bool anchoring_eligible = true;
    static std::unique_ptr<Slic3r::Fill> create_fill() { return std::make_unique<FakeFillType>(); }
    static bool use_bridge_flow() { return false; }
};

struct FakeAnchoringFalseFeature {
    static constexpr int id = 900002;
    static constexpr std::string_view key = "fake-anchoring-false";
    static constexpr std::string_view label = "FakeAnchoringFalse";
    static constexpr bool anchoring_eligible = false;
    static std::unique_ptr<Slic3r::Fill> create_fill() { return std::make_unique<FakeFillType>(); }
    static bool use_bridge_flow() { return false; }
};

using FakeAnchoringRegistry = Slic3r::Boss::BossFillRegistry<FakeAnchoringTrueFeature, FakeAnchoringFalseFeature>;
} // namespace

TEST_CASE("BossFillRegistry::anchoring_eligible dispatches per-feature, not unconditionally", "[boss][fill]")
{
    REQUIRE(FakeAnchoringRegistry::anchoring_eligible(FakeAnchoringTrueFeature::id) == true);
    REQUIRE(FakeAnchoringRegistry::anchoring_eligible(FakeAnchoringFalseFeature::id) == false);
}

TEST_CASE("BossFillRegistry::create dispatches to the right feature's Fill subtype", "[boss][fill]")
{
    std::unique_ptr<Slic3r::Fill> f = FakeAnchoringRegistry::create(FakeAnchoringTrueFeature::id);
    REQUIRE(f != nullptr);
    REQUIRE(dynamic_cast<FakeFillType*>(f.get()) != nullptr);
}

TEST_CASE("BossFillRegistry::create rejects an id no compiled-in feature owns", "[boss][fill]")
{
    std::unique_ptr<Slic3r::Fill> f = FakeAnchoringRegistry::create(999999);
    REQUIRE(f == nullptr);
}
