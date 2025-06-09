#include "Slic3r/Biz/Expr/Eval.hpp"

#include <sstream>

#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/get.hpp>
#include <fmt/format.h>


namespace Slic3r::Biz::Expr {


namespace {

using namespace Domain::Expr;

struct ValuePrinter : boost::static_visitor<std::ostream&>
{
    explicit ValuePrinter(std::ostream& os) : os(os) {}

    std::ostream& operator()(bool val)
    {
        os << val;
        return os;
    }

    std::ostream& operator()(double val)
    {
        os << val;
        return os;
    }

    std::ostream& operator()(const std::string& val)
    {
        os << "\"" << val << "\"" ;
        return os;
    }

    std::ostream& operator()(const RegEx& val)
    {
        os << "/" << val.source() << "/" ;
        return os;
    }
private:
    std::ostream& os;
};

std::ostream& operator<<(std::ostream& os, const Value& v)
{
    ValuePrinter printer(os);
    boost::apply_visitor(printer, v);
    return os;
}

template <typename T>
T safe_get(const Value& v, const char* op_name)
{
    if (v.type() != typeid(T)) {
        std::ostringstream os;
        os << v;

        throw EvalError(fmt::format(
            "Operation {} expecting value of type {} but instead have value {} of type {}",
            op_name, v.type().name(), os.str(), typeid(T).name()
        ));
    }
    return boost::get<T>(v);
}


struct Evaluator : boost::static_visitor<Value>
{
    Evaluator(const ValueMap& vars, const FuncMap& funcs)
        : m_vars(vars), m_functions(funcs)
    {}

    Value operator()(const std::string& v) const { return v; }
    Value operator()(const RegEx& v) const { return v; }
    Value operator()(double v) const { return v; }
    Value operator()(bool v) const { return v; }
    Value operator()(const Binary& v) const
    {
        Value lhs = boost::apply_visitor(*this, v.left);

        // lazy evaluation: making this function instead of directly evaluating it here
        // is an optimization for cases like (true || rhs) and (false && rhs) where the rhs
        // evaluation can be skipped.
        auto eval_rhs = [&]() {
            return boost::apply_visitor(*this, v.right);
        };

        switch (v.op)
        {
        case BinaryOp::Add:
            return safe_get<double>(lhs, "+") + safe_get<double>(eval_rhs(), "+");
        case BinaryOp::Subtract:
            return safe_get<double>(lhs, "-") - safe_get<double>(eval_rhs(), "-");
        case BinaryOp::Multiply:
            return safe_get<double>(lhs, "*") * safe_get<double>(eval_rhs(), "*");
        case BinaryOp::Divide:
            return safe_get<double>(lhs, "/") / safe_get<double>(eval_rhs(), "/");

        case BinaryOp::Eq:
            return lhs == eval_rhs();
        case BinaryOp::NotEq:
            return lhs != eval_rhs();
        case BinaryOp::Lt:
            return lhs < eval_rhs();
        case BinaryOp::Gt:
            return lhs > eval_rhs();
        case BinaryOp::LtEq:
            return lhs <= eval_rhs();
        case BinaryOp::GtEq:
            return lhs >= eval_rhs();

        case BinaryOp::And:
            return safe_get<bool>(lhs, "&&") && safe_get<bool>(eval_rhs(), "&&");
        case BinaryOp::Or:
            return safe_get<bool>(lhs, "||") || safe_get<bool>(eval_rhs(), "||");
        case BinaryOp::RegExMatch:
            return safe_get<RegEx>(eval_rhs(), "=~").match(safe_get<std::string>(lhs, "=~"));
        }

        UNREACHABLE("Unknown binary op");
    }

    Value operator()(const Unary& v) const
    {
        Value expr = boost::apply_visitor(*this, v.expr);
        // TODO: type mismatched error handling
        switch (v.op) {
        case UnaryOp::Not:
            return !safe_get<bool>(expr, "!");
        case UnaryOp::Plus:
            return expr;
        case UnaryOp::Minus:
            return -safe_get<double>(expr, "unary -");
        }

        UNREACHABLE("Unknown unary op");
    }

    Value operator()(const VarRef& v) const
    {
        auto it = m_vars.find(v.name);
        if (it == m_vars.end())
            throw EvalError(fmt::format("Unknown variable '{}'", v.name));
        return it->second;
    }

    Value operator()(const FuncCall& v) const
    {
        auto it = m_functions.find(v.name);
        if (it == m_functions.end())
            throw EvalError(fmt::format("Unknown function '{}'", v.name));
        ValueList args;
        for (const auto& arg : v.args)
            args.emplace_back(boost::apply_visitor(*this, arg));
        return it->second(args);
    }

private:
    const ValueMap& m_vars;
    const FuncMap& m_functions;
};


}



Value Eval::eval(const Expr& expr, const ValueMap& extra_vars) const
{
    ValueMap vars = m_vars;
    for (const auto& [k, v] : extra_vars)
        vars[k] = v;

    Evaluator evaluator(vars, m_functions);
    return boost::apply_visitor(evaluator, expr);
}


}
