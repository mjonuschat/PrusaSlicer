#include "Slic3r/Biz/Expr/Parser.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

using namespace Slic3r::Domain::Expr;

namespace Slic3r::Domain::Expr {
std::ostream& operator<<(std::ostream& os, const Binary& rhs)
{
    os << "Binary expression: " << rhs.left << " " << int(rhs.op) << " " << rhs.right << std::endl;
    return os;
}

std::ostream& operator<<(std::ostream& os, const UnaryOp& rhs)
{
    switch (rhs) {
    case UnaryOp::Plus:
        return os << "+";
    case UnaryOp::Minus:
        return os << "-";
    case UnaryOp::Not:
        return os << "!";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const BinaryOp& rhs)
{
    switch (rhs) {
    case BinaryOp::Add:
        return os << "+";
    case BinaryOp::Subtract:
        return os << "-";
    case BinaryOp::Multiply:
        return os << "*";
    case BinaryOp::Divide:
        return os << "/";
    case BinaryOp::And:
        return os << "&&";
    case BinaryOp::Or:
        return os << "||";
    case BinaryOp::Eq:
        return os << "==";
    case BinaryOp::NotEq:
        return os << "!=";
    case BinaryOp::Lt:
        return os << "<";
    case BinaryOp::Gt:
        return os << ">";
    case BinaryOp::LtEq:
        return os << "<=";
    case BinaryOp::GtEq:
        return os << ">=";
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const Unary& rhs)
{
    os << "Unary expression: " << int(rhs.op) << " " << rhs.expr << std::endl;
    return os;
}

std::ostream& operator<<(std::ostream& os, const VarRef& rhs)
{
    os << "Var Ref: " << rhs.name << std::endl;
    return os;
}


std::ostream& operator<<(std::ostream& os, const FuncCall& rhs)
{
    os << "Func Call: " << rhs.name << "(";
    bool first = true;
    for (const auto& arg : rhs.args) {
        if (!first)
            os << ", ";
        else
            first = false;
        os << arg;
    }
    os << ")" << std::endl;
    return os;
}

struct ExprAstPrinter : boost::static_visitor<std::ostream&>
{
    std::ostream& os;
    explicit ExprAstPrinter(std::ostream& os) : os(os) {}
    template <typename T>
    std::ostream& operator()(const T& var) const
    { os << var; return os; }
};

std::ostream& operator<<(std::ostream& os, const ExprAst& rhs)
{
    boost::apply_visitor(ExprAstPrinter(os), rhs);
    return os;
}
}


BOOST_FUSION_ADAPT_STRUCT(Binary,
    (BinaryOp, op)
    (ExprAst, left)
    (ExprAst, right)
)

BOOST_FUSION_ADAPT_STRUCT(Unary,
    (UnaryOp, op)
    (ExprAst, expr)
)

BOOST_FUSION_ADAPT_STRUCT(FuncCall,
    (std::string, name)
    (std::vector<ExprAst>, args)
)

BOOST_FUSION_ADAPT_STRUCT(VarRef,
    (std::string, name)
)

namespace Slic3r::Biz::Expr {

namespace {
template<typename Iterator>
struct ExprParser : qi::grammar<Iterator, ExprAst(), ascii::space_type>
{
    ExprParser() : ExprParser::base_type(expr)
    {
        using qi::float_;
        using qi::lexeme;
        using qi::lit;
        using ascii::alpha;
        using ascii::alnum;

        func_call = qi::as_string[lexeme[alpha >> *alnum]] >> '(' >> -(expr % ',') >> ')';
        var_ref = qi::as_string[lexeme[alpha >> *alnum]];

        factor = float_
            | func_call
            | var_ref
            | '(' >> expr >> ')';
            ;

        term = factor[ qi::_val = qi::_1 ] >>
            *( ('*' >> factor)[ qi::_val = phx::construct<Binary>(BinaryOp::Multiply, qi::_val, qi::_1)]
             | ('/' >> factor)[ qi::_val = phx::construct<Binary>(BinaryOp::Divide, qi::_val, qi::_1)]
             );
        expr = term[ qi::_val = qi::_1 ] >>
            *( ('+' >> term)[ qi::_val = phx::construct<Binary>(BinaryOp::Add, qi::_val, qi::_1)]
             | ('-' >> term)[ qi::_val = phx::construct<Binary>(BinaryOp::Subtract, qi::_val, qi::_1)]
             );

        expr.name("expr");
        term.name("term");
        factor.name("factor");
        func_call.name("func_call");
        var_ref.name("var_ref");

        debug(expr);
        debug(term);
        debug(factor);
        debug(func_call);
        debug(var_ref);
    }

    qi::rule<Iterator, ExprAst(), ascii::space_type> expr, term, factor;
    qi::rule<Iterator, FuncCall(), ascii::space_type> func_call;
    qi::rule<Iterator, VarRef(), ascii::space_type> var_ref;
};

} // namespace

ExprAst Parser::parse(std::string_view source)
{
    ExprParser<std::string_view::const_iterator> parser;
    ExprAst expr;
    bool success = qi::phrase_parse(source.begin(), source.end(), parser, ascii::space, expr);
    if (!success)
        throw std::runtime_error("Failed to parse expression");
    return  expr;
}

}