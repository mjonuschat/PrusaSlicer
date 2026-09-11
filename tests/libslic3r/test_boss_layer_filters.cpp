#include <catch2/catch_test_macros.hpp>

#include "boss/foundation/BossLayerFilters.hpp"

using namespace Slic3r::Boss;

namespace {

struct UppercaseFilter {
    static constexpr int id = 900101;
    static std::string filter_layer(std::string gcode)
    {
        for (char &c : gcode) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
        return gcode;
    }
};

struct AppendMarkerFilter {
    static constexpr int id = 900102;
    static std::string filter_layer(std::string gcode) { return std::move(gcode) + "; DONE\n"; }
};

struct NoopFilter {
    static constexpr int id = 900103;
};

} // namespace

TEST_CASE("BossLayerFilters folds filter_layer in declaration order", "[boss][layerfilters]")
{
    using Filters = BossLayerFilters<UppercaseFilter, AppendMarkerFilter, NoopFilter>;
    std::string result = Filters::filter_layer("g1 x1\n");
    CHECK(result == "G1 X1\n; DONE\n");
}

TEST_CASE("BossLayerFilters with no features is a no-op pass-through", "[boss][layerfilters]")
{
    using Empty = BossLayerFilters<>;
    CHECK(Empty::filter_layer("g1 x1\n") == "g1 x1\n");
}
