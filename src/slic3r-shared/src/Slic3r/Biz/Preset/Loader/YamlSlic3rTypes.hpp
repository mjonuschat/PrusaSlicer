#pragma once

#include "Slic3r/Domain/Expr/ExprAst.hpp"
#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Biz/Expr/Parser.hpp"
#include "Slic3r/Biz/Preset/Loader/Yaml.hpp"



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
        auto* token = fy_node_get_start_token(node.node);
        auto* mark = fy_token_start_mark(token);
        return Slic3r::Domain::Preset::SourceLocation{
            std::string{node.file}, size_t(mark->line + 1), size_t(mark->column + 1)
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


}
