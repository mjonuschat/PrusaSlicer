#include <catch2/catch_test_macros.hpp>

#include "Slic3r/Biz/Parser/PlaceholderParser.hpp"

using namespace Slic3r::Biz::Parser;

TEST_CASE("BOSS zero-pads month/day/hour/minute/second placeholders", "[boss][placeholder]")
{
    PlaceholderParser parser;
    parser.update_timestamp();
    for (const char* key : {"month", "day", "hour", "minute", "second"}) {
        const std::string value = parser.process(std::string("{") + key + "}", 0);
        INFO("key: " << key << " value: " << value);
        REQUIRE(value.size() == 2);
    }
}
