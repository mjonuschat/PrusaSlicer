#include "Slic3r/Biz/Config/FeatureStructurizer.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Slic3r::Biz::Config;
using namespace Slic3r::Domain;
using namespace Slic3r::Domain::Preset;

TEST_CASE("FeatureStructurizer", "[preset]")
{
    SECTION("Round trip test")
    {
        FeatureValueMap features;
        features["$.type.abbreviation"] = "PET";
        features["$.tags"] = JsonArray{JsonObject{{"id", "abrasive"}}};
        auto structure = features_to_structure(features);
        auto result = structure_to_features(structure);
        REQUIRE(result.size() == features.size());
        REQUIRE(result == features);
    }
}
