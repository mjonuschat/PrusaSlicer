#include <catch2/catch_test_macros.hpp>
#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Biz/Preset/Loader/Yaml.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

namespace Tests {
struct Ver
{
    int major{0};
    int minor{0};
    std::optional<int> patch;
};

struct Item
{
    std::string id;
};

struct MyData
{
    Ver version;
    int a{0};
    std::string b;

    std::vector<Item> items;
    std::optional<int> opt_int;
    std::variant<float, int, std::string> param;
};

}

STRUCT_DESC_SIMPLE(Tests::Ver, major, minor, patch);
STRUCT_DESC_SIMPLE(Tests::Item, id);
STRUCT_DESC_SIMPLE(Tests::MyData, version, a, b, items, opt_int, param);

TEST_CASE("Load Yaml from file into MyData")
{


    const std::string filename = Tests::get_datadir().string() + "/preset/test.yaml";
    Tests::MyData data;
    try {
        Yaml::Document doc = Yaml::parse_file(filename.c_str());
        data  = Yaml::parse_struct<Tests::MyData>(doc);
        REQUIRE(data.a == 42);
        REQUIRE(data.b == "answer");
        REQUIRE(data.version.major == 1);
        REQUIRE(data.version.minor == 12);
        REQUIRE(*data.version.patch == 321);
        REQUIRE(data.opt_int.has_value() == false);
        REQUIRE(data.items.size() == 3);
        REQUIRE(data.items[0].id == "1");
        REQUIRE(data.items[1].id == "2");
        REQUIRE(data.items[2].id == "3");
        REQUIRE(data.param.index() == 0);
        REQUIRE(std::get<float>(data.param) == Catch::Approx(3.14f));
    } catch (const Yaml::ParseError& e) {
        std::cerr << e.what() << std::endl;
    }

}

TEST_CASE("Load Yaml from string")
{
    std::string yaml = R"(
minor: 1
major: 1
)";
    Yaml::Document doc = Yaml::parse_string(yaml.c_str());
    REQUIRE_THROWS_WITH(Yaml::parse_struct<Tests::Ver>(doc), Catch::Matchers::ContainsSubstring("'minor'"));
    Tests::Ver ver = Yaml::parse_struct<Tests::Ver>(doc);
}


