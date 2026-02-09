#include <catch2/catch_test_macros.hpp>
#include <Slic3r/Biz/Expr/Simplify.hpp>
#include <Slic3r/Biz/Expr/Parser.hpp>
#include <boost/variant/get.hpp>
#include <iostream>

TEST_CASE("Simplify tests", "[expr]")
{
    using namespace Slic3r::Domain::Expr;
    using namespace Slic3r::Biz::Expr;

    Parser parser;

    SECTION("Absorb")
    {
        for (const auto source :
             {"d >= 0.2 && d >= 0.2",
              "d >= 0.2 && (hf || d >= 0.2)",
              "d >= 0.2 || d >= 0.2",
              "d >= 0.2 || (hf && d >= 0.2)",
              "d >= 0.2 || (d >= 0.2 && (hf || d >= 0.2))"})
        {
            auto expr = parser.parse(source);
            auto simplified = simplify(expr);
            REQUIRE(equals_to(simplified, parser.parse("d >= 0.2")));
        }
    }

    SECTION("Redundant")
    {
        for (const auto source : {"hf && !!hf", "!!hf || hf"}) {
            auto expr = parser.parse(source);
            auto simplified = simplify(expr);
            REQUIRE(equals_to(simplified, parser.parse("hf")));
        }
    }
}