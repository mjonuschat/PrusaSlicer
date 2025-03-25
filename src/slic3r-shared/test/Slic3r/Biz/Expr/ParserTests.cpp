#include <catch2/catch_test_macros.hpp>
#include <Slic3r/Biz/Expr/Parser.hpp>
#include <boost/variant/get.hpp>
#include <iostream>

TEST_CASE("Parser tests")
{
    using namespace Slic3r::Domain::Expr;
    using namespace Slic3r::Biz::Expr;

    ExprAst expr;
    Parser parser;

    /* WIP */
    expr = parser.parse("3.0");
    REQUIRE(boost::get<float>(expr) == 3.0f);

    expr = parser.parse("printer.base_model");
    REQUIRE(boost::get<VarRef>(expr).name == "printer.base_model");

    expr = parser.parse("2 + 3 * 4");
    {
        const Binary root = boost::get<Binary>(expr);
        REQUIRE(root.op == BinaryOp::Add);
        REQUIRE(boost::get<float>(root.left) == 2);
        const Binary node = boost::get<Binary>(root.right);
        REQUIRE(node.op == BinaryOp::Multiply);
        REQUIRE(boost::get<float>(node.left) == 3);
        REQUIRE(boost::get<float>(node.right) == 4);
    }

    expr = parser.parse("2 / 3 - 4");
    {
        const Binary root = boost::get<Binary>(expr);
        REQUIRE(root.op == BinaryOp::Subtract);
        REQUIRE(boost::get<float>(root.right) == 4);
        const Binary node = boost::get<Binary>(root.left);
        REQUIRE(node.op == BinaryOp::Divide);
        REQUIRE(boost::get<float>(node.left) == 2);
        REQUIRE(boost::get<float>(node.right) == 3);
    }

    expr = parser.parse("func(1, \"a\", false)");
    {
        const FuncCall root = boost::get<FuncCall>(expr);
        REQUIRE(root.name == "func");
        REQUIRE(root.args.size() == 3);
        REQUIRE(boost::get<float>(root.args[0]) == 1.0f);
        REQUIRE(boost::get<std::string>(root.args[1]) == "a");
        REQUIRE(boost::get<bool>(root.args[2]) == false);
    }

    expr = parser.parse("\"hello\"");
    REQUIRE(boost::get<std::string>(expr) == "hello");

    expr = parser.parse("name == \"x\"");
    {
        const Binary root = boost::get<Binary>(expr);
        REQUIRE(root.op == BinaryOp::Eq);
        const VarRef node = boost::get<VarRef>(root.left);
        REQUIRE(node.name == "name");
        REQUIRE(boost::get<std::string>(root.right) == "x");
    }

    expr = parser.parse("+3");
    {
        const Unary root = boost::get<Unary>(expr);
        REQUIRE(root.op == UnaryOp::Plus);
        REQUIRE(boost::get<float>(root.expr) == 3);
    }

    for (std::string_view source : {"not true", "!true"}) {
        expr = parser.parse(source);
        {
            const Unary root = boost::get<Unary>(expr);
            REQUIRE(root.op == UnaryOp::Not);
            REQUIRE(boost::get<bool>(root.expr) == true);
        }
    }

    for (std::string_view source : {"x && y", "x and y"}) {
        expr = parser.parse(source);
        {
            const auto root = boost::get<Binary>(expr);
            REQUIRE(root.op == BinaryOp::And);
            REQUIRE(boost::get<VarRef>(root.left).name == "x");
            REQUIRE(boost::get<VarRef>(root.right).name == "y");
        }
    }

    for (std::string_view source : {"x || y", "x or y"}) {
        expr = parser.parse(source);
        {
            const auto root = boost::get<Binary>(expr);
            REQUIRE(root.op == BinaryOp::Or);
            REQUIRE(boost::get<VarRef>(root.left).name == "x");
            REQUIRE(boost::get<VarRef>(root.right).name == "y");
        }
    }

    for (auto [source, op] : {
        std::make_tuple("val < 3 + sum(x, 4)", BinaryOp::Lt),
        std::make_tuple("val > 3 + sum(x, 4)", BinaryOp::Gt),
        std::make_tuple("val <= 3 + sum(x, 4)", BinaryOp::LtEq),
        std::make_tuple("val >= 3 + sum(x, 4)", BinaryOp::GtEq),
        std::make_tuple("val == 3 + sum(x, 4)", BinaryOp::Eq),
        std::make_tuple("val != 3 + sum(x, 4)", BinaryOp::NotEq),
    }) {
        expr = parser.parse(source);
        {
            const auto root = boost::get<Binary>(expr);
            REQUIRE(root.op == op);
            REQUIRE(boost::get<VarRef>(root.left).name == "val");
            const auto node1 = boost::get<Binary>(root.right);
            REQUIRE(node1.op == BinaryOp::Add);
            REQUIRE(boost::get<float>(node1.left) == 3);
            const auto node2 = boost::get<FuncCall>(node1.right);
            REQUIRE(node2.name == "sum");
            REQUIRE(node2.args.size() == 2);
            REQUIRE(boost::get<VarRef>(node2.args[0]).name == "x");
            REQUIRE(boost::get<float>(node2.args[1]) == 4);
        }
    }

    expr = parser.parse("a + 3 < val and val < b * 6");
    {
        const auto root = boost::get<Binary>(expr);
        REQUIRE(root.op == BinaryOp::And);

        const auto node_left = boost::get<Binary>(root.left);
        REQUIRE(node_left.op == BinaryOp::Lt);

        const auto node_left_left = boost::get<Binary>(node_left.left);
        REQUIRE(node_left_left.op == BinaryOp::Add);
        REQUIRE(boost::get<VarRef>(node_left_left.left).name == "a");
        REQUIRE(boost::get<float>(node_left_left.right) == 3);

        REQUIRE(boost::get<VarRef>(node_left.right).name == "val");

        const auto node_right = boost::get<Binary>(root.right);
        REQUIRE(node_right.op == BinaryOp::Lt);
        REQUIRE(boost::get<VarRef>(node_right.left).name == "val");
        const auto node_right_right = boost::get<Binary>(node_right.right);
        REQUIRE(node_right_right.op == BinaryOp::Multiply);
        REQUIRE(boost::get<VarRef>(node_right_right.left).name == "b");
        REQUIRE(boost::get<float>(node_right_right.right) == 6);
    }

    for (std::string_view source : {"not a or b", "! a || b"}) {
        expr = parser.parse(source);
        {
            const auto root = boost::get<Binary>(expr);
            REQUIRE(root.op == BinaryOp::Or);
            const auto left = boost::get<Unary>(root.left);
            REQUIRE(left.op == UnaryOp::Not);
            REQUIRE(boost::get<VarRef>(left.expr).name == "a");
            REQUIRE(boost::get<VarRef>(root.right).name == "b");
        }
    }

    for (std::string_view source : {"not (a or b)", "! (a || b)"}) {
        expr = parser.parse(source);
        {
            const auto root = boost::get<Unary>(expr);
            REQUIRE(root.op == UnaryOp::Not);
            const auto node = boost::get<Binary>(root.expr);
            REQUIRE(node.op == BinaryOp::Or);
            REQUIRE(boost::get<VarRef>(node.left).name == "a");
            REQUIRE(boost::get<VarRef>(node.right).name == "b");
        }
    }
}
