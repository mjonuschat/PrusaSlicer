#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "boss/foundation/BossSeamVisibilityRegistry.hpp"
#include "libslic3r/Point.hpp"

using namespace Slic3r::Boss;
using Slic3r::Vec3f;

namespace {
struct FakeBiasFeature {
    static constexpr int id = 900601;

    static void modify_visibility(const SeamVisibilityContext &ctx)
    {
        for (float &v : ctx.visibility)
            v += 1.0f;
    }
};

struct FakeNoOpFeature {
    static constexpr int id = 900602;
};
} // namespace

TEST_CASE(
    "BossSeamVisibilityRegistry calls every feature that implements modify_visibility",
    "[boss][seam]"
)
{
    std::vector<float> visibility{0.0f, 0.5f};
    std::vector<Vec3f> normals{Vec3f(0, -1, 0), Vec3f(0, 1, 0)};

    using Registry = BossSeamVisibilityRegistry<FakeBiasFeature>;
    Registry::modify_visibility(SeamVisibilityContext{visibility, normals, nullptr});

    CHECK(visibility[0] == 1.0f);
    CHECK(visibility[1] == 1.5f);
}

TEST_CASE(
    "BossSeamVisibilityRegistry is a no-op when no feature implements modify_visibility",
    "[boss][seam]"
)
{
    std::vector<float> visibility{0.25f};
    std::vector<Vec3f> normals{Vec3f(0, -1, 0)};

    using Registry = BossSeamVisibilityRegistry<FakeNoOpFeature>;
    Registry::modify_visibility(SeamVisibilityContext{visibility, normals, nullptr});

    CHECK(visibility[0] == 0.25f);
}

TEST_CASE("BossSeamVisibilityRegistry composes multiple features additively", "[boss][seam]")
{
    std::vector<float> visibility{0.0f};
    std::vector<Vec3f> normals{Vec3f(0, -1, 0)};

    using Registry = BossSeamVisibilityRegistry<FakeBiasFeature, FakeNoOpFeature>;
    Registry::modify_visibility(SeamVisibilityContext{visibility, normals, nullptr});

    CHECK(visibility[0] == 1.0f);
}
