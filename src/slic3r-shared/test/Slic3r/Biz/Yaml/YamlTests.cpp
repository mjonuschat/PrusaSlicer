#include <catch2/catch_test_macros.hpp>
#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Biz/Yaml/Yaml.hpp"
#include "Slic3r/Biz/Yaml/YamlSlic3rTypes.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

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

struct Condition
{
    Slic3r::Domain::Expr::ExprAst condition;
};

struct VecData
{
    std::vector<uint8_t> data;
};

struct ValueData
{
    Slic3r::Domain::Preset::PresetValueMap presets;
    Slic3r::Domain::Preset::FeatureValueMap features;
};

struct SourceLocatedData
{
    Slic3r::Domain::Preset::SourceLocated<std::string> name;
};

}

STRUCT_DESC_SIMPLE(Tests::Ver, major, minor, patch);
STRUCT_DESC_SIMPLE(Tests::Item, id);
STRUCT_DESC_SIMPLE(Tests::MyData, version, a, b, items, opt_int, param);



STRUCT_DESC_SIMPLE(Tests::Condition, condition);
STRUCT_DESC_SIMPLE(Tests::VecData, data);
STRUCT_DESC_SIMPLE(Tests::ValueData, presets, features);
STRUCT_DESC_SIMPLE(Tests::SourceLocatedData, name);

namespace Yaml = Slic3r::Biz::Yaml;

TEST_CASE("Load Yaml from file into MyData", "[yaml]")
{


    const std::string filename = Tests::get_datadir().string() + "/preset/test.yaml";
    Tests::MyData data;
    try {
        Yaml::YamlAdapter::Document doc = Yaml::parse_file(filename.c_str());
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

TEST_CASE("Load Yaml from string", "[yaml]")
{
    std::string yaml_ver_no_minor = R"(
major: 1
)";
    std::string yaml_ver_ok = R"(
major: 3
minor: 2
patch: 321
)";

    Yaml::YamlAdapter::Document doc = Yaml::parse_string(yaml_ver_no_minor);

    REQUIRE_THROWS_MATCHES(
        Yaml::parse_struct<Tests::Ver>(doc),
        Yaml::ParseError,
        //Catch::Matchers::ContainsSubstring("Required field 'minor' not found")
        Catch::Matchers::MessageMatches(Catch::Matchers::ContainsSubstring("Required field 'minor' not found"))
    );

    doc = Yaml::parse_string(yaml_ver_ok);
    Tests::Ver ver;
    REQUIRE_NOTHROW(ver = Yaml::parse_struct<Tests::Ver>(doc));
    REQUIRE(ver.major == 3);
    REQUIRE(ver.minor == 2);
    REQUIRE(ver.patch == 321);
}


TEST_CASE("ExprAst parsing", "[yaml]")
{
    std::string yaml = R"(
condition: 'tool.nozzle_diameter >= 0.2'
)";

    Yaml::YamlAdapter::Document doc = Yaml::parse_string(yaml);
    Tests::Condition condition = Yaml::parse_struct<Tests::Condition>(doc);
    REQUIRE(boost::get<Slic3r::Domain::Expr::Binary>(condition.condition).op == Slic3r::Domain::Expr::BinaryOp::GtEq);

}

TEST_CASE("Vector parsing", "[yaml]")
{
    std::string yaml = R"(
data: [0]
)";

    Yaml::YamlAdapter::Document doc = Yaml::parse_string(yaml);
    Tests::VecData vec = Yaml::parse_struct<Tests::VecData>(doc);
    REQUIRE(vec.data.size() == 1);
}

TEST_CASE("Slic3r Types", "[yaml]")
{
    using Slic3r::Domain::Preset::FeatureValue;
    using Slic3r::Domain::Preset::PresetValue;
    using Slic3r::Domain::Vec2d;
    using Slic3r::Domain::Vec2ds;
    using Slic3r::Domain::Preset::Bools;
    using Slic3r::Domain::Preset::Floats;
    using Slic3r::Domain::Preset::Strings;
    SECTION("PresetValue and FeatureValue")
    {
        std::string yaml = R"(
features:
  bool: true
  num: 42.1
  str: Hello world
presets:
  vec2: '0x0'
  vec2s: ['0x0']
  bool: true
  bools: [true, false]
  num: 42.1
  nums: [1, 2, 3]
  str: Hello
  strs: ['Hello', 'world']
  mono: null
)";
        try {
            auto doc = Yaml::parse_string(yaml);
            Tests::ValueData data = Yaml::parse_struct<Tests::ValueData>(doc);
            REQUIRE(data.features.find("bool")->second == FeatureValue{true});
            REQUIRE(data.features.find("num")->second == FeatureValue{42.1f});
            REQUIRE(data.features.find("str")->second == FeatureValue{"Hello world"});


            REQUIRE(data.presets.find("vec2")->second == PresetValue{Vec2d{0, 0}});
            REQUIRE(data.presets.find("vec2s")->second == PresetValue{Vec2ds{Vec2d{0, 0}}});
            REQUIRE(data.presets.find("bool")->second == PresetValue{true});
            REQUIRE(data.presets.find("bools")->second == PresetValue{Bools{true, false}});
            REQUIRE(data.presets.find("num")->second == PresetValue{42.1f});
            REQUIRE(data.presets.find("nums")->second == PresetValue{Floats{1, 2, 3}});
            REQUIRE(data.presets.find("str")->second == PresetValue{"Hello"});
            REQUIRE(data.presets.find("strs")->second == PresetValue{Strings{"Hello", "world"}});
            REQUIRE(data.presets.find("mono")->second == PresetValue{std::monostate{}});
        } catch (Yaml::ParseError& e) {
            std::cerr << e.what() << std::endl;
            FAIL(e.what());
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
            FAIL(e.what());
        }
    }
    SECTION("SourceLocated")
    {
        std::string yaml = R"(
# some comment
name: 'Abc'
)";
        try {
            auto doc = Yaml::parse_string(yaml);
            auto data = Yaml::parse_struct<Tests::SourceLocatedData>(doc);
            REQUIRE(data.name.value == "Abc");
            REQUIRE(data.name.source_location.column == 7);
            REQUIRE(data.name.source_location.line == 3);
            REQUIRE(data.name.source_location.file == "<string>");
        } catch (Yaml::ParseError& e) {
            std::cerr << e.what() << std::endl;
            FAIL(e.what());
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
            FAIL(e.what());
        }
    }
}