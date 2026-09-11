#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "boss/features/aligned-rear-seam/AlignedRearSeamFeature.hpp"
#include "boss/foundation/SeamVisibilityContext.hpp"
#include "libslic3r/Point.hpp"
#include "fff_print/test_data.hpp"

using namespace Slic3r::Boss;
using Slic3r::PrintConfigView;
using Slic3r::Test::TestConfig;
using Slic3r::Vec3f;

namespace {

PrintConfigView aligned_rear_enabled_config_view()
{
    TestConfig config;
    config.print.items.opt("seam_position_aligned_rear").set(true);
    return config.get_view();
}

} // namespace

TEST_CASE(
    "AlignedRearSeamFeature leaves visibility unchanged when the option is off",
    "[boss][seam]"
)
{
    std::vector<float> visibility{0.5f};
    std::vector<Vec3f> normals{Vec3f(0.0f, -1.0f, 0.0f)};

    AlignedRearSeamFeature::modify_visibility(SeamVisibilityContext{visibility, normals, nullptr});

    CHECK(visibility[0] == 0.5f);
}

TEST_CASE("AlignedRearSeamFeature adds the fixed rear bias when the option is on", "[boss][seam]")
{
    const PrintConfigView config_view = aligned_rear_enabled_config_view();

    std::vector<float> visibility{0.0f, 0.0f};
    std::vector<Vec3f> normals{Vec3f(0.0f, -1.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f)};

    AlignedRearSeamFeature::modify_visibility(
        SeamVisibilityContext{visibility, normals, &config_view}
    );

    // Facing the front, -Y (normal = (0,-1,0)): maximal penalty, pushed away.
    CHECK(visibility[0] == 1.0f);
    // Facing the rear, +Y (normal = (0,1,0)): minimal penalty, left preferred.
    CHECK(visibility[1] == Catch::Approx(0.1f));
}

TEST_CASE("AlignedRearSeamFeature never produces a negative or >1 bias", "[boss][seam]")
{
    const PrintConfigView config_view = aligned_rear_enabled_config_view();

    std::vector<float> visibility{0.0f};
    std::vector<Vec3f> normals{Vec3f(0.0f, -1.0f, 0.0f)};

    AlignedRearSeamFeature::modify_visibility(
        SeamVisibilityContext{visibility, normals, &config_view}
    );

    CHECK(visibility[0] <= 1.0f);
    CHECK(visibility[0] >= 0.0f);
}
