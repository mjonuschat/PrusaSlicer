#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <vector>

#include "boss/foundation/BossPerimeterGeometryRegistry.hpp"
#include "libslic3r/GCode/ExtrusionOrder.hpp"

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"

using namespace Slic3r;
using namespace Slic3r::Boss;
using Slic3r::GCode::ExtrusionOrder::Perimeter;

namespace {

// Minimal single-extruder ConfigView, following FullConfigFDM::defaults()'s
// own fixture shape (this branch has no public helper that builds one, since
// Slic3r::Test::build_fff_printer_config is private to slic3r-domain-tests).
Domain::ConfigView make_config_view()
{
    Domain::ConfigPackFDM config_pack;

    Domain::Preset::HwPrinterConfig hw_config{
        .technology = Domain::PrinterTechnology::FFF,
        .tool_count = 1,
        .tools      = {Domain::Preset::HwToolConfig{.features = {{"nozzle_diameter", 0.4}}}},
    };

    auto full_config = std::make_shared<Domain::FullConfigFDM>(config_pack, std::vector<unsigned>{0}, hw_config);
    Domain::ConfigView view{full_config, {}};
    view.finalize();
    return view;
}

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

    const Domain::ConfigView config_view = make_config_view();
    CHECK(Registry::suppress_staggering(config_view, std::optional<int>{1}) == true);
}

TEST_CASE("BossPerimeterGeometryRegistry's suppress_staggering is false when no feature implements it", "[boss][seam]")
{
    using Registry = BossPerimeterGeometryRegistry<FakeNoOpFeature>;

    const Domain::ConfigView config_view = make_config_view();
    CHECK(Registry::suppress_staggering(config_view, std::optional<int>{1}) == false);
}
