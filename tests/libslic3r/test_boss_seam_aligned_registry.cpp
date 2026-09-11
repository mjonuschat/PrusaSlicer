#include <catch2/catch_test_macros.hpp>

#include "libslic3r/GCode/SeamAligned.hpp"

using namespace Slic3r::Seams::Aligned;

TEST_CASE("LeastVisiblePoint and SeamCandidate are visible outside SeamAligned.cpp", "[boss][seam]")
{
    LeastVisiblePoint p{};
    p.visibility = 0.5;
    CHECK(p.visibility == 0.5);

    SeamCandidate c{};
    CHECK(c.choices.empty());
    CHECK(c.visibilities.empty());
}

#include "boss/foundation/BossSeamAlignedRegistry.hpp"

using namespace Slic3r;
using namespace Slic3r::Boss;
using namespace Slic3r::Seams;
using namespace Slic3r::Seams::Aligned;

namespace {
struct FakeOverrideFeature {
    static constexpr int id = 900701;

    static std::optional<std::vector<Vec2d>> get_starting_positions_override(
        const Shells::Shell<> &, const Params &)
    {
        return std::vector<Vec2d>{Vec2d(1.0, 2.0)};
    }
};

struct FakePostprocessFeature {
    static constexpr int id = 900702;

    static void postprocess_shell_choices(
        const Shells::Shell<> &, std::vector<SeamChoice> &choices, const Params &)
    {
        for (SeamChoice &c : choices)
            c.position += Vec2d(10.0, 0.0);
    }
};

struct FakeNoOpFeature {
    static constexpr int id = 900703;
};
} // namespace

TEST_CASE(
    "BossSeamAlignedRegistry returns the first feature's starting-position override", "[boss][seam]"
)
{
    using Registry = BossSeamAlignedRegistry<FakeNoOpFeature, FakeOverrideFeature>;
    Shells::Shell<> shell;
    Params params{};

    auto result = Registry::get_starting_positions_override(shell, params);
    REQUIRE(result.has_value());
    REQUIRE(result->size() == 1);
    CHECK((*result)[0] == Vec2d(1.0, 2.0));
}

TEST_CASE(
    "BossSeamAlignedRegistry's starting-position override is nullopt when no feature implements it",
    "[boss][seam]"
)
{
    using Registry = BossSeamAlignedRegistry<FakeNoOpFeature>;
    Shells::Shell<> shell;
    Params params{};

    CHECK_FALSE(Registry::get_starting_positions_override(shell, params).has_value());
}

TEST_CASE(
    "BossSeamAlignedRegistry's get_seam_candidate override is nullopt when no feature implements it",
    "[boss][seam]"
)
{
    using Registry = BossSeamAlignedRegistry<FakeNoOpFeature>;
    Shells::Shell<> shell;
    Params params{};
    std::vector<std::vector<double>> precalculated_visibility;
    std::vector<LeastVisiblePoint> least_visible_points;
    SeamChoiceVisibility visibility_calculator =
        [](const SeamChoice &, const Perimeters::Perimeter &) { return 0.0; };

    CHECK_FALSE(Registry::get_seam_candidate_override(
        shell, Vec2d::Zero(), visibility_calculator, params, precalculated_visibility,
        least_visible_points)
        .has_value());
}

TEST_CASE(
    "BossSeamAlignedRegistry runs every feature's postprocess_shell_choices in sequence",
    "[boss][seam]"
)
{
    using Registry = BossSeamAlignedRegistry<FakeNoOpFeature, FakePostprocessFeature>;
    Shells::Shell<> shell;
    Params params{};
    std::vector<SeamChoice> choices{SeamChoice{0, 0, Vec2d(0.0, 0.0)}};

    Registry::postprocess_shell_choices(shell, choices, params);

    CHECK(choices[0].position == Vec2d(10.0, 0.0));
}
