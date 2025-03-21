#pragma once

#include <vector>
#include <boost/variant/recursive_variant.hpp>

namespace Slic3r::Domain::Expr {

struct Binary;
struct Unary;
struct FuncCall;
struct VarRef;

using ExprAst = boost::variant<
    float,
    std::string,
    boost::recursive_wrapper<Binary>,
    boost::recursive_wrapper<FuncCall>,
    boost::recursive_wrapper<VarRef>
>;

enum class BinaryOp
{
    Add, Subtract, Multiply, Divide,
    Eq, NotEq, Lt, LtEq, Gt, GtEq,
    And, Or,
};

struct Binary
{
    BinaryOp op;
    ExprAst left;
    ExprAst right;
};

enum class UnaryOp
{
    Plus, Minus, Not
};

struct Unary
{
    UnaryOp op;
    ExprAst expr;
};

struct FuncCall
{
    std::string name;
    std::vector<ExprAst> args;
};

struct VarRef
{
    std::string name;
};

} // namespace Slic3r::Domain::Expr
