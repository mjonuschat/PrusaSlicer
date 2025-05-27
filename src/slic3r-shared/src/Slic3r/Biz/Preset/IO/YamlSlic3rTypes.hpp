#pragma once

#include "Slic3r/Domain/Expr/ExprAst.hpp"
#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Biz/Expr/Parser.hpp"
#include "Slic3r/Biz/Preset/IO/Yaml.hpp"

namespace Yaml::Details {

template <>
struct TypeTraits<Slic3r::Domain::Expr::ExprAst>
{
    using ExprAst = Slic3r::Domain::Expr::ExprAst;
    using Parser = Slic3r::Biz::Expr::Parser;
    static ExprAst parse(const YamlAdapter::NodeRef& node)
    {
        static Parser parser;
        try {
            return parser.parse(get_node_scalar(node));
        } catch (Slic3r::Biz::Expr::ParseError& e) {
            throw ParseError(node, e.what());
        }
    }
};

template <>
struct TypeTraits<Slic3r::Domain::Preset::SourceLocation>
{
    using SourceLocation = Slic3r::Domain::Preset::SourceLocation;
    static SourceLocation parse(const YamlAdapter::NodeRef& node)
    {
        auto mark = YamlAdapter::mark(node);
        return Slic3r::Domain::Preset::SourceLocation{
            std::string{node.file}, mark.line, mark.column
        };
    }
};

template <typename T>
struct TypeTraits<Slic3r::Domain::Preset::SourceLocated<T>>
{
    using SourceLocated = Slic3r::Domain::Preset::SourceLocated<T>;
    using SourceLocation = Slic3r::Domain::Preset::SourceLocation;

    static SourceLocated parse(const YamlAdapter::NodeRef& node)
    {
        SourceLocated ret{TypeTraits<T>::parse(node), TypeTraits<SourceLocation>::parse(node)};
        return ret;
    }
};

template <>
struct TypeTraits<Slic3r::Domain::Vec2d>
{
    using Vec2d = Slic3r::Domain::Vec2d;
    static Vec2d parse(const YamlAdapter::NodeRef& node)
    {
        namespace qi = boost::spirit::qi;

        auto value = get_node_scalar(node);
        auto pos = value.find('x');
        if (pos == std::string::npos)
            throw ParseError(node, fmt::format("Invalid Vec2d '{}'", value));

        Vec2d ret;
        if (!qi::parse(std::cbegin(value), std::cbegin(value) + pos, qi::double_, ret.x()))
            throw ParseError(node, fmt::format("Invalid Vec2d value: '{}'", value));
        if (!qi::parse(std::cbegin(value) + pos + 1, std::cend(value), qi::double_, ret.y()))
            throw ParseError(node, fmt::format("Invalid Vec2d value: '{}'", value));

        return ret;
    }

};


}
