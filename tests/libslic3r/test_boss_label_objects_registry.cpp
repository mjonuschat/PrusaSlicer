#include <catch2/catch_test_macros.hpp>

#include "boss/foundation/BossLabelObjectsRegistry.hpp"
#include "libslic3r/boss/gcode/labeling/LabelObjectsPolicyContext.hpp"

namespace {

struct LabelExtensionFeature {
    static std::vector<Slic3r::Boss::LabelObjectsDeclaration> declare(const Slic3r::Boss::LabelObjectsPolicyContext &)
    {
        return {{"skirt", "0,0", "0,0 10,0 10,10 0,10"}};
    }
};

} // namespace

TEST_CASE("BossLabelObjectsRegistry concatenates a feature's declarations", "[boss][labeling]")
{
    using Registry = Slic3r::Boss::BossLabelObjectsRegistry<LabelExtensionFeature>;
    Slic3r::Boss::LabelObjectsPolicyContext ctx;
    auto declarations = Registry::declare(ctx);
    REQUIRE(declarations.size() == 1);
    REQUIRE(declarations[0].name == "skirt");
    REQUIRE(declarations[0].center == "0,0");
    REQUIRE(declarations[0].polygon == "0,0 10,0 10,10 0,10");
}

TEST_CASE("BossLabelObjectsRegistry with zero features returns nothing", "[boss][labeling]")
{
    using Registry = Slic3r::Boss::BossLabelObjectsRegistry<>;
    Slic3r::Boss::LabelObjectsPolicyContext ctx;
    REQUIRE(Registry::declare(ctx).empty());
}
