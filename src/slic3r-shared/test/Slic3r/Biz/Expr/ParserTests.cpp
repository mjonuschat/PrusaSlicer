#include <catch2/catch_test_macros.hpp>
#include <Slic3r/Biz/Expr/Parser.hpp>
#include <boost/variant/get.hpp>

TEST_CASE("Parse simple expr 1")
{
    using namespace Slic3r::Domain::Expr;
    using namespace Slic3r::Biz::Expr;

    ExprAst expr;
    Parser parser;

    /* WIP */
    expr = parser.parse("3.0");
    REQUIRE(boost::get<float>(expr) == 3.0f);

    expr = parser.parse("var");
    REQUIRE(boost::get<VarRef>(expr).name == "var");



}
