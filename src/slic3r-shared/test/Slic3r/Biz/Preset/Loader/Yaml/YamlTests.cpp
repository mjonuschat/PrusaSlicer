#include <catch2/catch_test_macros.hpp>
#include "Slic3r/TestUtils/TestData.hpp"
#include "Slic3r/Biz/Preset/Loader/Yaml.hpp"
#include "Slic3r/Biz/Preset/Loader/YamlSlic3rTypes.hpp"

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


}

STRUCT_DESC_SIMPLE(Tests::Ver, major, minor, patch);
STRUCT_DESC_SIMPLE(Tests::Item, id);
STRUCT_DESC_SIMPLE(Tests::MyData, version, a, b, items, opt_int, param);



STRUCT_DESC_SIMPLE(Tests::Condition, condition);
STRUCT_DESC_SIMPLE(Tests::VecData, data);

/*
template <typename T>
struct ExceptionSubstringMatcher : Catch::Matchers::MatcherBase<T>//Catch::Matchers::MatcherGenericBase
{
    explicit ExceptionSubstringMatcher(std::string substring)
        : m_substring(std::move(substring))
    {}

    bool match(const T& e) const
    {
        std::string msg = e.what();
        return msg.find(m_substring) != std::string::npos;
    }

protected:
    std::string describe() const override
    {
        return "Matches substring '" + m_substring + "'";
    }

private:
    std::string m_substring;
};

template <typename T>
ExceptionSubstringMatcher<T> exception_substring(const std::string& substring)
{
    return ExceptionSubstringMatcher<T>(substring);
}
*/
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
    std::string yaml_ver_no_minor = R"(
major: 1
)";
    std::string yaml_ver_ok = R"(
major: 3
minor: 2
patch: 321
)";

    Yaml::Document doc = Yaml::parse_string(yaml_ver_no_minor.c_str());

    REQUIRE_THROWS_MATCHES(
        Yaml::parse_struct<Tests::Ver>(doc),
        Yaml::ParseError,
        //Catch::Matchers::ContainsSubstring("Required field 'minor' not found")
        Catch::Matchers::MessageMatches(Catch::Matchers::ContainsSubstring("Required field 'minor' not found"))
    );

    doc = Yaml::parse_string(yaml_ver_ok.c_str());
    Tests::Ver ver;
    REQUIRE_NOTHROW(ver = Yaml::parse_struct<Tests::Ver>(doc));
    REQUIRE(ver.major == 3);
    REQUIRE(ver.minor == 2);
    REQUIRE(ver.patch == 321);
}


TEST_CASE("ExprAst parsing")
{
    std::string yaml = R"(
condition: 'tool.nozzle_diameter >= 0.2'
)";

    Yaml::Document doc = Yaml::parse_string(yaml.c_str());
    Tests::Condition condition = Yaml::parse_struct<Tests::Condition>(doc);
    REQUIRE(boost::get<Slic3r::Domain::Expr::Binary>(condition.condition).op == Slic3r::Domain::Expr::BinaryOp::GtEq);

}

TEST_CASE("Vector parsing")
{
    std::string yaml = R"(
data: [0]
)";

    Yaml::Document doc = Yaml::parse_string(yaml.c_str());
    Tests::VecData vec = Yaml::parse_struct<Tests::VecData>(doc);
    REQUIRE(vec.data.size() == 1);

}