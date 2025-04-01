#include "Slic3r/Biz/Expr/Parser.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/spirit/repository/include/qi.hpp>
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
    case BinaryOp::RegExMatch:
        return os << "=~";
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
    ExprParser() : ExprParser::base_type(logical_or)
    {
        using qi::float_;
        using qi::char_;
        using qi::lexeme;
        using qi::lit;
        using qi::string;
        using ascii::alpha;
        using ascii::alnum;

        auto kw = boost::spirit::repository::qi::distinct(qi::copy(alnum | '_'));

        raw_identifier = (alpha | char_('_')) >> *(alnum | char_('_'));
        identifier =
            qi::as_string[
                lexeme[raw_identifier]
            ];

        bool_constant = lexeme[
              qi::string("true")  [ qi::_val = true ]
            | qi::string("false") [ qi::_val = false ]
        ];

        string_constant %= lexeme[qi::as_string[
            qi::omit[lit('"')] >>
            *(('\\' >> char_) | (char_ - '"')) >>
            qi::omit[lit('"')]
        ]];

        regex_constant %= lexeme[qi::as_string[
            qi::omit[lit('/')] >>
            qi::raw[
                *( (lit('\\') >> char_) | (char_ - '/') )
            ] >>
            qi::omit[lit('/')]
        ]];


        func_call = qi::as_string[lexeme[raw_identifier]] >> '(' >> -(logical_or % ',') >> ')';

        var_ref = qi::as_string[
            lexeme[
                raw_identifier
                >> *(
                    char_('.') >> raw_identifier
                )
            ]
        ];
        factor = float_
            | bool_constant
            | string_constant
            | regex_constant
            | func_call
            | var_ref
            | '(' >> logical_or >> ')'
            ;

        unary =
              ( (lit('+') >> unary) [ qi::_val = phx::construct<Unary>(UnaryOp::Plus, qi::_1) ]
              | (lit('-') >> unary) [ qi::_val = phx::construct<Unary>(UnaryOp::Minus, qi::_1) ]
              | ( (lit('!') | kw["not"]) >> unary) [ qi::_val = phx::construct<Unary>(UnaryOp::Not, qi::_1) ]
              )
            | factor [ qi::_val = qi::_1 ]
            ;


        multiplicative = unary [ qi::_val = qi::_1 ] >>
            *( (char_('*') >> unary) [ qi::_val = phx::construct<Binary>(BinaryOp::Multiply, qi::_val, qi::_2) ]
             | (char_('/') >> unary) [ qi::_val = phx::construct<Binary>(BinaryOp::Divide, qi::_val, qi::_2) ]
             );

        additive = multiplicative [ qi::_val = qi::_1 ] >>
            *( (char_('+') >> multiplicative) [ qi::_val = phx::construct<Binary>(BinaryOp::Add, qi::_val, qi::_2) ]
             | (char_('-') >> multiplicative) [ qi::_val = phx::construct<Binary>(BinaryOp::Subtract, qi::_val, qi::_2) ]
             );

        relational = additive [ qi::_val = qi::_1 ] >>
            *( (string("<=") >> additive) [ qi::_val = phx::construct<Binary>(BinaryOp::LtEq, qi::_val, qi::_2) ]
             | (string(">=") >> additive) [ qi::_val = phx::construct<Binary>(BinaryOp::GtEq, qi::_val, qi::_2) ]
             | (char_('<') >> additive) [ qi::_val = phx::construct<Binary>(BinaryOp::Lt, qi::_val, qi::_2) ]
             | (char_('>') >> additive) [ qi::_val = phx::construct<Binary>(BinaryOp::Gt, qi::_val, qi::_2) ]
             | (string("=~") >> regex_constant) [qi::_val = phx::construct<Binary>(
                    BinaryOp::RegExMatch, qi::_val, phx::construct<RegEx>(qi::_2)
                )]
             );

        equality = relational [ qi::_val = qi::_1 ] >>
            *( (string("==") >> relational) [ qi::_val = phx::construct<Binary>(BinaryOp::Eq, qi::_val, qi::_2) ]
             | (string("!=") >> relational) [ qi::_val = phx::construct<Binary>(BinaryOp::NotEq, qi::_val, qi::_2) ]
             );

        logical_and = equality [ qi::_val = qi::_1 ] >>
            *(
                ((string("&&") | kw["and"]) >> equality)[ qi::_val = phx::construct<Binary>(BinaryOp::And, qi::_val, qi::_2) ]
             );

        logical_or = logical_and [ qi::_val = qi::_1 ] >>
            *(
                ((string("||") | kw["or"]) >> logical_and) [ qi::_val = phx::construct<Binary>(BinaryOp::Or, qi::_val, qi::_2) ]
             );
        factor.name("factor");
        unary.name("unary");
        multiplicative.name("multiplicative");
        additive.name("additive");
        relational.name("relational");
        equality.name("equality");
        logical_and.name("logical_and");
        logical_or.name("logical_or");
        bool_constant.name("bool_constant");
        string_constant.name("string_constant");
        func_call.name("func_call");
        var_ref.name("var_ref");
        regex_constant.name("regex_constant");

#if 0
        debug(factor);
        debug(unary);
        debug(multiplicative);
        debug(additive);
        debug(relational);
        debug(equality);
        debug(logical_and);
        debug(logical_or);
        debug(bool_constant);
        debug(string_constant);
        debug(func_call);
        debug(var_ref);
        debug(regex_constant);
#endif
    }

    qi::rule<Iterator, ExprAst(), ascii::space_type> logical_or, logical_and, equality, relational,
        additive, multiplicative, unary, factor;
    qi::rule<Iterator, FuncCall(), ascii::space_type> func_call;
    qi::rule<Iterator, VarRef(), ascii::space_type> var_ref;
    qi::rule<Iterator, bool(), ascii::space_type> bool_constant;
    qi::rule<Iterator, std::string(), ascii::space_type> string_constant, identifier;
    qi::rule<Iterator, RegEx(), ascii::space_type> regex_constant;
    qi::rule<Iterator, std::vector<char>()> raw_identifier;
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