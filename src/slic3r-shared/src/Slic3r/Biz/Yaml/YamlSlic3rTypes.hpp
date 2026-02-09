#pragma once

#include "Slic3r/Domain/Percentage.hpp"
#include "Slic3r/Domain/Expr/ExprAst.hpp"
#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Biz/Expr/Parser.hpp"
#include "Slic3r/Biz/Yaml/Yaml.hpp"

namespace Slic3r::Biz::Yaml::Details {

template <>
struct TypeTraits<Slic3r::Domain::Expr::ExprAst>
{
    using ExprAst = Slic3r::Domain::Expr::ExprAst;
    using Parser  = Slic3r::Biz::Expr::Parser;

    static Result<ExprAst> parse(const YamlAdapter::NodeRef& node)
    {
        static Parser parser;
        try {
            auto node_value = get_node_scalar(node);
            if (!node_value.has_value())
                return ResultError(node_value.error());
            return parser.parse(*node_value);
        } catch (Slic3r::Biz::Expr::ParseError& e) {
            return ResultError(ParseErrorDesc(node, e.what()));
        }
    }

    static std::optional<YamlAdapter::NodeRef> serialize(const ExprAst& val)
    {
        return YamlAdapter::create_scalar_node(to_string(val));
    }

};

template <>
struct TypeTraits<Slic3r::Domain::Preset::SourceLocation>
{
    using SourceLocation = Slic3r::Domain::Preset::SourceLocation;

    static Result<SourceLocation> parse(const YamlAdapter::NodeRef& node)
    {
        auto mark = YamlAdapter::mark(node);
        return Slic3r::Domain::Preset::SourceLocation{std::string{node.file}, mark.line, mark.column};
    }

    static std::optional<YamlAdapter::NodeRef> serialize(const SourceLocation&)
    {
        return std::nullopt;
    }
};

template <typename T>
struct TypeTraits<Slic3r::Domain::Preset::SourceLocated<T>>
{
    using SourceLocated  = Slic3r::Domain::Preset::SourceLocated<T>;
    using SourceLocation = Slic3r::Domain::Preset::SourceLocation;

    static Result<Slic3r::Domain::Preset::SourceLocated<T>> parse(const YamlAdapter::NodeRef& node)
    {
        auto data = TypeTraits<T>::parse(node);
        if (!data.has_value())
            return ResultError(data.error());

        auto source_location = TypeTraits<SourceLocation>::parse(node);
        if (!source_location.has_value())
            return ResultError(source_location.error());

        SourceLocated ret{*data, *source_location};
        return ret;
    }

    static std::optional<YamlAdapter::NodeRef> serialize(const SourceLocated& v)
    {
        return TypeTraits<T>::serialize(v.value);
    }
};

template <>
struct TypeTraits<Slic3r::Domain::Vec2d>
{
    using Vec2d = Slic3r::Domain::Vec2d;

    static Result<Vec2d> parse(const YamlAdapter::NodeRef& node)
    {
        namespace qi = boost::spirit::qi;

        Vec2d ret;

        auto node_value = get_node_scalar(node);
        if (!node_value.has_value())
            return ResultError(node_value.error());
        auto value = *node_value;
        auto pos   = value.find('x');
        if (pos == std::string::npos)
            return ResultError(ParseErrorDesc(node, fmt::format("Invalid Vec2d value '{}'", value)));

        auto it = std::cbegin(value);
        // parse first coordinate
        if (!qi::parse(it, std::cbegin(value) + pos, qi::double_, ret.x())
            || it != std::cbegin(value) + pos)
            return ResultError(ParseErrorDesc(node, fmt::format("Invalid Vec2d value: '{}'", value)));
        // skip the 'x' marker
        ++it;
        // parse second coordinate
        if (!qi::parse(it, std::cend(value), qi::double_, ret.y()) || it != std::cend(value))
            return ResultError(ParseErrorDesc(node, fmt::format("Invalid Vec2d value: '{}'", value)));

        return ret;
    }

    static std::optional<YamlAdapter::NodeRef> serialize(const Vec2d& val)
    {
        return YamlAdapter::create_scalar_node(fmt::format("{}x{}", val.x(), val.y()));
    }
};

template <>
struct TypeTraits<Slic3r::Domain::Percentage>
{
    using Percentage = Slic3r::Domain::Percentage;

    static Result<Percentage> parse(const YamlAdapter::NodeRef& node)
    {
        namespace qi = boost::spirit::qi;

        Percentage ret;

        auto node_value = get_node_scalar(node);
        if (!node_value.has_value())
            return ResultError(node_value.error());
        auto value = *node_value;
        auto pos   = value.find('%');
        bool valid = pos != std::string::npos
            && std::all_of(value.cbegin() + pos + 1, value.cend(), [](char c) {
            return std::isspace(c);
        });
        if (valid) {
            valid = qi::parse(value.cbegin(), value.cbegin() + pos, qi::double_, ret.value);
        }

        if (!valid)
            return ResultError(
                ParseErrorDesc(node, fmt::format("Invalid Percentage value '{}'", value))
            );

        return ret;
    }

    static std::optional<YamlAdapter::NodeRef> serialize(const Percentage& val)
    {
        return YamlAdapter::create_scalar_node(fmt::format("{}%", val.value));
    }
};

template <>
struct TypeTraits<Slic3r::Domain::FloatOrPercentage>
{
    using FloatOrPercentage = Domain::FloatOrPercentage;
    using Percentage = Domain::Percentage;

    static Result<FloatOrPercentage> parse(const YamlAdapter::NodeRef& node)
    {
        auto val = TypeTraits<Percentage>::parse(node);
        if (val.has_value())
            return val.value();
        return TypeTraits<double>::parse(node);
    }

    static std::optional<YamlAdapter::NodeRef> serialize(const FloatOrPercentage& val)
    {
        if (val.is_percentage())
            return TypeTraits<Percentage>::serialize(val.percentage());
        return TypeTraits<double>::serialize(val.float_value());
    }
};

template <>
struct TypeTraits<Domain::JsonObject>;

Result<Domain::JsonValue> parse_json_value(const YamlAdapter::NodeRef& node);
std::optional<YamlAdapter::NodeRef> serialize_json_value(const Domain::JsonValue& val);

template <>
struct TypeTraits<Domain::JsonArray>
{
    using JsonArray = Domain::JsonArray;
    static Result<JsonArray> parse(const YamlAdapter::NodeRef& node)
    {
        YAML_HANDLE_ENSURE(ensure_node_type(node, NodeType::Sequence));
        JsonArray ret;
        const size_t n = YamlAdapter::sequence_item_count(node);
        ret.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            auto node_ref = YamlAdapter::sequence_item_at(node, i);
            auto parsed_value = parse_json_value(node_ref);
            if (!parsed_value.has_value())
                return unexpected{parsed_value.error()};
            ret.push_back(parsed_value.value());
        }
        return ret;
    }

    static std::optional<YamlAdapter::NodeRef> serialize(const JsonArray& val)
    {
        auto node = YamlAdapter::create_sequence_node();
        for (const auto& item : val) {
            auto item_node = serialize_json_value(item);
            if (!item_node.has_value())
                return std::nullopt;
            YamlAdapter::sequence_append(node, item_node.value());
        }
        return node;
    }
};

template <>
struct TypeTraits<Domain::JsonObject>
{
    using JsonObject = Domain::JsonObject;
    static Result<JsonObject> parse(const YamlAdapter::NodeRef& node)
    {
        YAML_HANDLE_ENSURE(ensure_node_type(node, NodeType::Mapping));
        JsonObject ret;
        const size_t n = YamlAdapter::mapping_item_count(node);
        for (size_t i = 0; i < n; ++i) {
            auto key_value_pair = YamlAdapter::mapping_key_value_at(node, i);
            YamlAdapter::NodeRef value_ref = YamlAdapter::value(key_value_pair, node);
            auto parsed_value = parse_json_value(value_ref);
            if (!parsed_value.has_value())
                return unexpected{parsed_value.error()};
            auto parsed_key = TypeTraits<std::string>::parse(YamlAdapter::key(key_value_pair, node));
            if (!parsed_key.has_value())
                return unexpected{parsed_key.error()};
            ret.emplace(std::make_pair(parsed_key.value(), parsed_value.value()));
        }
        return ret;
    }

    static std::optional<YamlAdapter::NodeRef> serialize(const JsonObject& val)
    {
        auto node = YamlAdapter::create_mapping_node();
        for (const auto& [k, v] : val) {
            auto value_node = serialize_json_value(v);
            if (!value_node.has_value())
                return std::nullopt;
            auto key_node = YamlAdapter::create_scalar_node(k);
            YamlAdapter::mapping_append(node, key_node, value_node.value());
        }
        return node;

    }
};

template <>
struct TypeTraits<Domain::JsonValue>
{
    static Result<Domain::JsonValue> parse(const YamlAdapter::NodeRef& node)
    {
        return parse_json_value(node);
    }

    static std::optional<YamlAdapter::NodeRef> serialize(const Domain::JsonValue& val)
    {
        return serialize_json_value(val);
    }
};


} // namespace Slic3r::Biz::Yaml::Details
