#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <vector>

#include "boss/foundation/BossPerimeterGeometryRegistry.hpp"
#include "libslic3r/GCode/ExtrusionOrder.hpp"

using namespace Slic3r;
using namespace Slic3r::Boss;
using Slic3r::GCode::ExtrusionOrder::Perimeter;

namespace {
struct FakeAppendFeature {
    static constexpr int id = 900801;

    static void modify_perimeters(const PerimeterGeometryContext &ctx)
    {
        ctx.perimeters.push_back(Perimeter{});
    }

    static bool suppress_staggering(const Domain::ConfigView &, std::optional<int> perimeter_index)
    {
        return perimeter_index.has_value() && *perimeter_index == 1;
    }
};

struct FakeNoOpFeature {
    static constexpr int id = 900802;
};
} // namespace

TEST_CASE("BossPerimeterGeometryRegistry calls every feature's modify_perimeters", "[boss][seam]")
{
    std::vector<Perimeter> perimeters;

    using Registry = BossPerimeterGeometryRegistry<FakeAppendFeature>;
    Registry::modify_perimeters(PerimeterGeometryContext{perimeters, nullptr, 0});

    CHECK(perimeters.size() == 1);
}

TEST_CASE("BossPerimeterGeometryRegistry's modify_perimeters is a no-op when no feature implements it", "[boss][seam]")
{
    std::vector<Perimeter> perimeters;

    using Registry = BossPerimeterGeometryRegistry<FakeNoOpFeature>;
    Registry::modify_perimeters(PerimeterGeometryContext{perimeters, nullptr, 0});

    CHECK(perimeters.empty());
}

TEST_CASE("BossPerimeterGeometryRegistry's suppress_staggering ORs every feature's answer", "[boss][seam]")
{
    using Registry = BossPerimeterGeometryRegistry<FakeNoOpFeature, FakeAppendFeature>;

    CHECK(Registry::suppress_staggering(*static_cast<const Domain::ConfigView *>(nullptr), std::optional<int>{1}) == true);
}

TEST_CASE("BossPerimeterGeometryRegistry's suppress_staggering is false when no feature implements it", "[boss][seam]")
{
    using Registry = BossPerimeterGeometryRegistry<FakeNoOpFeature>;

    CHECK(Registry::suppress_staggering(*static_cast<const Domain::ConfigView *>(nullptr), std::optional<int>{1}) == false);
}
