#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <vector>

#include "boss/foundation/BossWipeTowerSpeedCapRegistry.hpp"

namespace Slic3r { class PrintConfigView; }

using namespace Slic3r::Boss;

namespace {

struct FixtureCapFeature {
    static constexpr int id = 900801;
    static std::optional<float> speed_cap(const Slic3r::PrintConfigView &, const std::vector<unsigned> &)
    {
        return 42.f;
    }
};

struct FixtureNoCapFeature {
    static constexpr int id = 900802;
    static std::optional<float> speed_cap(const Slic3r::PrintConfigView &, const std::vector<unsigned> &)
    {
        return std::nullopt;
    }
};

struct FixtureNoopFeature {
    static constexpr int id = 900803;
};

struct FixtureOtherCapFeature {
    static constexpr int id = 900804;
    static std::optional<float> speed_cap(const Slic3r::PrintConfigView &, const std::vector<unsigned> &)
    {
        return 7.f;
    }
};

} // namespace

TEST_CASE("BossWipeTowerSpeedCapRegistry collects every feature's cap", "[boss][wipe_tower]")
{
    const Slic3r::PrintConfigView *config = nullptr;
    const std::vector<unsigned> extruder_candidates{};

    SECTION("no features yields an empty vector")
    {
        using Registry = BossWipeTowerSpeedCapRegistry<>;
        REQUIRE(Registry::collect(*config, extruder_candidates).empty());
    }

    SECTION("a feature declining a cap contributes nothing")
    {
        using Registry = BossWipeTowerSpeedCapRegistry<FixtureNoCapFeature>;
        REQUIRE(Registry::collect(*config, extruder_candidates).empty());
    }

    SECTION("a feature with no speed_cap method at all is a no-op")
    {
        using Registry = BossWipeTowerSpeedCapRegistry<FixtureNoopFeature>;
        REQUIRE(Registry::collect(*config, extruder_candidates).empty());
    }

    SECTION("a feature with a cap contributes exactly one value")
    {
        using Registry = BossWipeTowerSpeedCapRegistry<FixtureCapFeature>;
        REQUIRE(Registry::collect(*config, extruder_candidates) == std::vector<float>{42.f});
    }

    SECTION("multiple features each contribute their own cap")
    {
        using Registry = BossWipeTowerSpeedCapRegistry<
            FixtureCapFeature, FixtureNoCapFeature, FixtureNoopFeature, FixtureOtherCapFeature>;
        const std::vector<float> caps = Registry::collect(*config, extruder_candidates);
        REQUIRE(caps.size() == 2);
        REQUIRE(caps[0] == 42.f);
        REQUIRE(caps[1] == 7.f);
    }
}
