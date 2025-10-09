#pragma once

#include <stdexcept>
#include <cstddef>
#include <map>
#include <memory>
#include <utility>
#include <variant>
#include <vector>
#include <optional>
#include <ranges>
#include <utility>
#include <string_view>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/spirit/include/qi.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <magic_enum/magic_enum.hpp>

#include "Slic3r/expected.hpp"

#if defined(SLIC3R_YAML_LIBFYAML)
#include "YamlAdapterLibfyaml.hpp"
#elif defined(SLIC3R_YAML_YAMLCPP)
#include "YamlAdapterYamlCpp.hpp"
#else
#error "No YAML library selected"
#endif

namespace Slic3r::Biz::Yaml {

#if defined(SLIC3R_YAML_LIBFYAML)
using YamlAdapter = Libfyaml::YamlAdapterLibfyaml;
#elif defined(SLIC3R_YAML_YAMLCPP)
using YamlAdapter = YamlCpp::YamlAdapterYamlCpp;
#else
#error "No YAML library selected"
#endif

/**
 * @brief Parse YAML from file
 * @param file_name A file to parse
 * @return A parsed YAML document
 */
YamlAdapter::Document parse_file(const char* file_name);

/**
 * @brief Parse YAML from string
 * @param yaml YAML source as std::string_view
 * @return A parsed YAML document
 */
YamlAdapter::Document parse_string(std::string_view yaml);

/**
 * @brief Parse all YAML documents from file
 * @param file_name A file to parse
 * @param parse_doc A parse function to be called for parsed each document
 */
void parse_all_documents_in_file(
    const char* file_name,
    const std::function<void(const YamlAdapter::Document&)>& parse_doc
);

/**
 * @brief Parse all YAML documents from file
 * @param yaml YAML source as std::string_view
 * @param parse_doc A parse function to be called for parsed each document
 */
void parse_all_documents_in_string(
    std::string_view yaml,
    const std::function<void(const YamlAdapter::Document&)>& parse_doc
);

namespace Details {

inline std::string describe_node(const YamlAdapter::NodeRef& node)
{
    auto mark = YamlAdapter::mark(node);
    return fmt::format("[file: {}:{}:{}]", node.file, mark.line, mark.column);
}

} // namespace Details

struct ParseErrorDesc
{
    const Details::Mark mark;
    const std::string node_description;
    const std::string message;

    ParseErrorDesc(const YamlAdapter::NodeRef& node, std::string message) :
        mark(YamlAdapter::mark(node)),
        node_description(Details::describe_node(node)),
        message(std::move(message))
    {}
};

template <typename T>
using Result      = expected<T, ParseErrorDesc>;
using ResultError = unexpected<ParseErrorDesc>;

struct ParseError : std::runtime_error
{
    ParseError(const YamlAdapter::NodeRef& node, const std::string& msg) :
        std::runtime_error(Details::describe_node(node) + ": " + msg)
    {}

    explicit ParseError(const ParseErrorDesc& desc) :
        std::runtime_error(desc.node_description + ": " + desc.message)
    {}

    ParseError(const ParseError&) = default;
    ParseError(ParseError&&)      = default;

    ParseError& operator=(const ParseError&) = default;
    ParseError& operator=(ParseError&&)      = default;

    ParseError decorate_with_source(const std::string& source) const
    {
        return ParseError("[" + source + "] " + what());
    }

private:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}

    explicit ParseError(const char* msg) : std::runtime_error(msg) {}
};

template <typename T>
T value_or_throw(const Result<T>& result)
{
    if (result.has_value())
        return result.value();
    throw ParseError(result.error());
}

/**
 * @}
 */

namespace Details {
template <typename T>
struct StructTraits
{};
} // namespace Details

template <typename T>
Result<typename Details::StructTraits<T>::Type> parse_struct(const YamlAdapter::NodeRef& node);

namespace Details {
inline std::string node_type_value(const NodeType type)
{
    switch (type) {
    case NodeType::Scalar:
        return "scalar";
    case NodeType::Sequence:
        return "sequence";
    case NodeType::Mapping:
        return "mapping";
    }
    return "unknown";
}

#define YAML_HANDLE_ENSURE(opt) if ((opt).has_value()) return ResultError{(opt).value()};

inline std::optional<ParseErrorDesc> ensure_node_not_null(const YamlAdapter::NodeRef& node)
{
    if (node.is_null())
        return ParseErrorDesc(node, "Null is not allowed here");
    return std::nullopt;
}

inline std::optional<ParseErrorDesc> ensure_node_type(const YamlAdapter::NodeRef& node, NodeType type)
{
    auto node_type = YamlAdapter::node_type(node);
    if (node_type != type)
        return ParseErrorDesc(
            node,
            std::string("Node type mismatch, expecting '")
                + node_type_value(type)
                + "' but got '"
                + node_type_value(node_type)
                + "'"
        );
    return std::nullopt;
}

inline Result<std::string_view> get_node_scalar(const YamlAdapter::NodeRef& node)
{
    YAML_HANDLE_ENSURE(ensure_node_type(node, NodeType::Scalar));
    YAML_HANDLE_ENSURE(ensure_node_not_null(node));
    return YamlAdapter::scalar_value(node);
}

template <typename T, typename Enabled = void>
struct TypeTraits
{};

template <>
struct TypeTraits<bool>
{
    static Result<bool> parse(const YamlAdapter::NodeRef& node) noexcept
    {
        auto value = get_node_scalar(node);
        if (value.has_value()) {
            if (*value == "true")
                return true;
            if (*value == "false")
                return false;
        } else
            return ResultError{value.error()};

        return ResultError{
            {node,
             fmt::format("Invalid bool value: '{}', allowed values are 'true' and 'false'", *value)}
        };
    }
};

template <typename T, typename P>
Result<T> parse_with_spirit(const YamlAdapter::NodeRef& node, P parser)
{
    auto value = get_node_scalar(node);
    if (!value.has_value())
        return ResultError{value.error()};
    T ret;
    namespace qi = boost::spirit::qi;
    auto it      = std::cbegin(*value);
    if (!qi::parse(it, std::cend(*value), parser, ret) || it != std::cend(*value))
        return ResultError{{node, fmt::format("Invalid {} value: '{}'", typeid(T).name(), *value)}};
    return ret;
}

#define TYPE_TRAITS_WITH_SPIRIT_PARSE(T, P)                     \
template <>                                                     \
struct TypeTraits<T>                                            \
{                                                               \
    static Result<T> parse(const YamlAdapter::NodeRef& node)    \
    {                                                           \
        return parse_with_spirit<T>(node, P);                   \
    }                                                           \
};

TYPE_TRAITS_WITH_SPIRIT_PARSE(float, boost::spirit::qi::float_);
TYPE_TRAITS_WITH_SPIRIT_PARSE(double, boost::spirit::qi::double_);
TYPE_TRAITS_WITH_SPIRIT_PARSE(uint8_t, boost::spirit::qi::uint_);
TYPE_TRAITS_WITH_SPIRIT_PARSE(uint16_t, boost::spirit::qi::uint_);
TYPE_TRAITS_WITH_SPIRIT_PARSE(uint32_t, boost::spirit::qi::uint_);
TYPE_TRAITS_WITH_SPIRIT_PARSE(uint64_t, boost::spirit::qi::uint_);
TYPE_TRAITS_WITH_SPIRIT_PARSE(int8_t, boost::spirit::qi::int_);
TYPE_TRAITS_WITH_SPIRIT_PARSE(int16_t, boost::spirit::qi::int_);
TYPE_TRAITS_WITH_SPIRIT_PARSE(int32_t, boost::spirit::qi::int_);

#undef TYPE_TRAITS_WITH_SPIRIT_PARSE

template <>
struct TypeTraits<std::string>
{
    static Result<std::string> parse(const YamlAdapter::NodeRef& node)
    {
        auto value = get_node_scalar(node);
        return value.transform([](auto&& v) { return std::string(v); });
    }
};

template <typename, typename = void>
struct HasTypeTraits : std::false_type
{};

template <typename T>
struct HasTypeTraits<
    T,
    std::void_t<decltype(TypeTraits<T>::parse(std::declval<const YamlAdapter::NodeRef&>()))>> :
    std::true_type
{};

template <typename T>
struct TypeTraits<std::optional<T>, std::enable_if_t<HasTypeTraits<T>::value>>
{
    static Result<std::optional<T>> parse(const YamlAdapter::NodeRef& node)
    {
        if (node && !node.is_null()) {
            auto val = TypeTraits<T>::parse(node);
            if (val.has_value())
                return *val;

            return ResultError{val.error()};
        }
        return std::nullopt;
    }
};

template <typename T>
struct TypeTraits<std::vector<T>, std::enable_if_t<HasTypeTraits<T>::value>>
{
    static Result<std::vector<T>> parse(const YamlAdapter::NodeRef& node)
    {
        YAML_HANDLE_ENSURE(ensure_node_type(node, NodeType::Sequence));
        const size_t n = YamlAdapter::sequence_item_count(node);
        std::vector<T> ret;
        ret.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            Result<T> element = TypeTraits<T>::parse(YamlAdapter::sequence_item_at(node, i));
            if (!element.has_value())
                return ResultError{element.error()};
            ret.push_back(*element);
        }
        return ret;
    }
};

template <typename K, typename V>
struct TypeTraits<std::map<K, V>, std::enable_if_t<HasTypeTraits<K>::value && HasTypeTraits<V>::value>>
{
    static Result<std::map<K, V>> parse(const YamlAdapter::NodeRef& node)
    {
        YAML_HANDLE_ENSURE(ensure_node_type(node, NodeType::Mapping));
        const size_t n = YamlAdapter::mapping_item_count(node);
        std::map<K, V> ret;
        for (size_t i = 0; i < n; ++i) {
            auto kv_pair    = YamlAdapter::mapping_key_value_at(node, i);
            auto key_node   = YamlAdapter::key(kv_pair, node);
            auto value_node = YamlAdapter::value(kv_pair, node);
            ;
            Result<K> key = TypeTraits<K>::parse(key_node);
            if (!key.has_value())
                return ResultError{key.error()};
            Result<V> value = TypeTraits<V>::parse(value_node);
            if (!value.has_value())
                return ResultError{value.error()};
            ret.emplace(std::move(*key), std::move(*value));
        }
        return ret;
    }
};

template <typename V, typename T>
Result<V> parse_variant(const YamlAdapter::NodeRef& node)
{
    return TypeTraits<T>::parse(node);
}

template <typename V, typename T, typename... Ts, std::enable_if_t<(sizeof...(Ts) > 0), int> = 0>
Result<V> parse_variant(const YamlAdapter::NodeRef& node)
{
    Result<T> val = TypeTraits<T>::parse(node);
    if (val.has_value())
        return val;
    return parse_variant<V, Ts...>(node);
}

template <typename... Ts>
struct AllHasTypeTraits : std::bool_constant<(HasTypeTraits<Ts>::value && ...)>
{};

template <typename... Ts>
struct TypeTraits<std::variant<Ts...>, std::enable_if_t<AllHasTypeTraits<Ts...>::value>>
{
    using ValueType = std::variant<Ts...>;

    static Result<ValueType> parse(const YamlAdapter::NodeRef& node)
    {
        return parse_variant<ValueType, Ts...>(node);
    }
};

template <>
struct TypeTraits<std::monostate>
{
    using ValueType = std::monostate;

    static Result<ValueType> parse(const YamlAdapter::NodeRef& node)
    {
        if (node.is_null())
            return {};
        return ResultError{ParseErrorDesc(node, "Node must be null")};
    }
};

template <typename F, typename = void>
struct FieldHasImplicitValue : std::false_type
{};

template <typename F>
struct FieldHasImplicitValue<F, std::void_t<decltype(F::implicit_value())>> : std::true_type
{};

template <typename T>
struct IsOptional : std::false_type
{};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type
{};

template <typename F>
struct FieldIsOptional : IsOptional<typename F::Type>
{};

template <typename F, typename = void>
struct FieldHasValidation : std::false_type
{};

template <typename F>
struct FieldHasValidation<F, std::void_t<decltype(F::validate(std::declval<typename F::Type>()))>> :
    std::true_type
{};

template <typename... Ts>
struct TypeList
{};

template <typename Field, std::enable_if_t<!FieldHasImplicitValue<Field>::value, int> = 0>
Result<typename Field::Type> parse_field(const YamlAdapter::NodeRef& node)
{
    return TypeTraits<typename Field::Type>::parse(node);
}

template <typename Field, std::enable_if_t<FieldHasImplicitValue<Field>::value, int> = 0>
Result<typename Field::Type> parse_field(const YamlAdapter::NodeRef& node)
{
    if (!node)
        return Field::implicit_value();
    else
        return TypeTraits<typename Field::Type>::parse(node);
}

template <typename Field, typename = void>
void validate_field(const typename Field::Type& type)
{}

template <typename Field, std::enable_if_t<FieldHasValidation<Field>::value>>
void validate_field(const typename Field::Type& field_value)
{
    Field::validate(field_value);
}

template <typename S, typename Field, typename = void>
std::optional<ParseErrorDesc> parse_field(S& s, const YamlAdapter::NodeRef& node)
{
    using FT          = typename Field::Type;
    auto* raw_storage = reinterpret_cast<char*>(&s) + Field::offset;
    FT& typed_storage = *reinterpret_cast<FT*>(raw_storage);
    auto val          = parse_field<Field>(node);
    if (!val.has_value())
        return val.error();
    typed_storage = std::move(val.value());
    validate_field<Field>(typed_storage);
    return std::nullopt;
}

template <
    typename Field,
    std::enable_if_t<!(FieldHasImplicitValue<Field>::value || FieldIsOptional<Field>::value), int> = 0>
Result<YamlAdapter::NodeRef> get_mapping_node_with_key(const YamlAdapter::NodeRef& node, const char* key)
{
    auto value_node = YamlAdapter::mapping_value_at(node, key);
    if (!value_node)
        return ResultError{ParseErrorDesc(node, fmt::format("Required field '{}' not found", key))};
    return value_node;
}

template <
    typename Field,
    std::enable_if_t<FieldHasImplicitValue<Field>::value || FieldIsOptional<Field>::value, int> = 0>
Result<YamlAdapter::NodeRef> get_mapping_node_with_key(const YamlAdapter::NodeRef& node, const char* key)
{
    return YamlAdapter::mapping_value_at(node, key);
}

template <typename T, typename F>
struct ParseFieldTypeList;

template <typename S, typename... Fs>
struct ParseFieldTypeList<S, TypeList<Fs...>>
{
    static std::optional<ParseErrorDesc> parse(S& s, const YamlAdapter::NodeRef& node)
    {
        if constexpr (sizeof...(Fs) == 0) {
            return std::nullopt;
        } else {
            return parse_fields<Fs...>(s, node);
        }
    }

private:
    template <typename F, typename... Rest>
    static std::optional<ParseErrorDesc> parse_fields(S& s, const YamlAdapter::NodeRef& node)
    {
        if constexpr (F::name == nullptr)
            return parse_fields_node<F, Rest...>(s, node, node);
        else {
            auto n = get_mapping_node_with_key<F>(node, F::name);
            if (!n.has_value())
                return n.error();
            return parse_fields_node<F, Rest...>(s, node, *n);
        }
    }

    template <typename F, typename... Rest>
    static std::optional<ParseErrorDesc> parse_fields_node(
        S& s,
        const YamlAdapter::NodeRef& node,
        const YamlAdapter::NodeRef& selected_node
    )
    {
        if (auto err = parse_field<S, F>(s, selected_node)) {
            return err;
        }
        if constexpr (sizeof...(Rest) > 0) {
            return parse_fields<Rest...>(s, node);
        }
        return std::nullopt;
    }
};

template <typename S>
Result<typename S::Type> parse_struct_helper(const YamlAdapter::NodeRef& node)
{
    typename S::Type ret;

    auto opt_err = ParseFieldTypeList<typename S::Type, typename S::Fields>::parse(ret, node);

    if (opt_err.has_value())
        return ResultError{opt_err.value()};

    return ret;
}

template <typename T>
Result<bool> try_parse_discriminated_struct(
    const YamlAdapter::NodeRef& node,
    std::string_view value,
    std::tuple<const char*, std::function<void(T&&)>> loader
)
{
    if (value == std::get<0>(loader)) {
        Result<T> s = parse_struct<T>(node);
        if (!s.has_value())
            return unexpected{s.error()};
        std::get<1>(loader)(std::move(*s));
        return true;
    }
    return false;
}

template <typename, typename = void>
struct HasStructTraits : std::false_type
{};

template <typename T>
struct HasStructTraits<T, std::void_t<typename StructTraits<T>::Type>> : std::true_type
{};

template <typename T>
struct TypeTraits<T, std::enable_if_t<HasStructTraits<T>::value>>
{
    static Result<T> parse(const YamlAdapter::NodeRef& node)
    {
        return parse_struct<T>(node);
    }
};

template <typename E, typename = void>
struct EnumTraits
{};

template <typename E>
struct EnumValue
{
    const char* name;
    const E value;

    constexpr EnumValue(const char* name, E value) noexcept : name(name), value(value) {}
};

template <typename, typename = void>
struct HasEnumTraits : std::false_type
{};

template <typename T>
struct HasEnumTraits<T, std::void_t<typename EnumTraits<T>::Type>> : std::true_type
{};

template <typename T>
struct TypeTraits<T, std::enable_if_t<std::is_enum_v<T> && !HasEnumTraits<T>::value>>
{
    static Result<T> parse(const YamlAdapter::NodeRef& node)
    {
        auto value = get_node_scalar(node);
        if (!value.has_value())
            return ResultError{value.error()};
        auto ret = magic_enum::enum_cast<T>(*value, magic_enum::case_insensitive);
        if (!ret.has_value()) {
            auto allowed_values = magic_enum::enum_names<T>();
            return ResultError{ParseErrorDesc{
                node,
                fmt::format(
                    "Invalid enum value: '{}', allowed values: {}",
                    *value,
                    fmt::join(allowed_values.begin(), allowed_values.end(), ", ")
                )
            }};
        }
        return ret.value();
    }
};

template <typename T>
struct TypeTraits<T, std::enable_if_t<HasEnumTraits<T>::value>>
{
    static Result<T> parse(const YamlAdapter::NodeRef& node)
    {
        auto value         = TypeTraits<std::string>::parse(node);
        const auto& values = EnumTraits<T>::values;
        auto it            = std::find_if(values.begin(), values.end(), [&](const auto& v) {
            return v.name == value;
        });
        if (it == values.end()) {
            auto keys = values | std::views::transform([](const EnumValue<T>& ev) -> const char* {
                return ev.name;
            });
            return ResultError{ParseErrorDesc{
                node,
                fmt::format("Invalid enum value: '{}', allowed values: {}", *value, fmt::join(keys, ","))
            }};
        }
        return it->value;
    }
};

} // namespace Details

/**
 * @name POD structures parsing API
 * @{
 */

/**
 * @brief Parse structure from yaml node
 * @tparam T Type of structure. Note that this structure needs STRUCT_DESC(...) definition.
 * @param node Yaml node representing the structure to load.
 * @return Loaded structure or ParseErrorDesc in `tl::expected`.
 */
template <typename T>
Result<typename Details::StructTraits<T>::Type> parse_struct(const YamlAdapter::NodeRef& node)
{
    return Details::parse_struct_helper<Details::StructTraits<T>>(node);
}

/**
 * @brief Parse structure from yaml node
 * @tparam T Type of structure. Note that this structure needs STRUCT_DESC(...) definition.
 * @param doc Yaml YamlAdapter::Document containing the structure to load.
 * @return Loaded structure or ParseErrorDesc in `tl::expected`.
 */
template <typename T>
Result<typename Details::StructTraits<T>::Type> parse_struct(const YamlAdapter::Document& doc)
{
    return parse_struct<T>(doc.root());
}

/**
 * @brief Parse structure from yaml node
 * @tparam T Type of structure. Note that this structure needs STRUCT_DESC(...) definition.
 * @param doc Yaml YamlAdapter::Document containing the structure to load.
 * @return Loaded structure or ParseErrorDesc in `tl::expected`.
 */
template <typename T>
typename Details::StructTraits<T>::Type parse_struct_unwrap(const YamlAdapter::Document& doc)
{
    auto ret = parse_struct<T>(doc);
    if (!ret.has_value())
        throw ParseError(ret.error());
    return ret.value();
}

/**
 * @brief Load struct from possible set of types distinguished by value in discriminator field.
 * @tparam Ts types of structs to load
 * @param node Yaml node containing structure to load and the discriminator_field_name
 * @param discriminator_field_name A name of field the type of structure is distinguished by
 * @param loaders List (variadic args) of two element tuples containing `const char*` value of
 * discriminator and function that will be called with loaded struct of given type.
 */
template <typename... Ts>
void parse_structs_by_discriminant(
    const YamlAdapter::NodeRef& node,
    const char* discriminator_field_name,
    const std::tuple<const char*, std::function<void(Ts&&)>>&... loaders
)
{
    auto discr_node  = YamlAdapter::mapping_value_at(node, discriminator_field_name);
    auto discr_value = Details::get_node_scalar(discr_node);

    if (!discr_value.has_value())
        throw ParseError(discr_value.error());

    if (!(value_or_throw(Details::try_parse_discriminated_struct(node, *discr_value, loaders)) || ...))
    {
        std::vector<const char*> discr_field_values;
        (discr_field_values.push_back(std::get<0>(loaders)), ...);
        throw ParseError(
            node,
            fmt::format(
                "Cannot parse any structure identified by field (discriminator) '{}' with value '{}'. Allowed values are: {}",
                discriminator_field_name,
                *discr_value,
                fmt::join(discr_field_values.begin(), discr_field_values.end(), ", ")
            )
        );
    }
}

/**
 * @brief Load struct from possible set of types distinguished by value in discriminator field.
 * @tparam Ts types of structs to load
 * @param doc Yaml YamlAdapter::Document containing structure to load and the discriminator_field_name
 * @param discriminator_field_name A name of field the type of structure is distinguished by
 * @param loaders List (variadic args) of two element tuples containing `const char*` value of
 * discriminator and function that will be called with loaded struct of given type.
 */
template <typename... Ts>
void parse_structs_by_discriminant(
    const YamlAdapter::Document& doc,
    const char* discriminator_field_name,
    const std::tuple<const char*, std::function<void(Ts&&)>>&... loaders
)
{
    parse_discriminated_structs(doc.root(), discriminator_field_name, loaders...);
}

} // namespace Slic3r::Biz::Yaml

// For each field, generate a struct with type, name, and offset.
#define DETAILS_STRUCT_DESC_FIELD(r, data, elem)                                            \
struct BOOST_PP_CAT(Field_, BOOST_PP_TUPLE_ELEM(0, elem)) {                                 \
    using Type = decltype(std::declval<data>().BOOST_PP_TUPLE_ELEM(0, elem));               \
    static constexpr const char* name = BOOST_PP_IF(                                        \
        BOOST_PP_IS_EMPTY(BOOST_PP_TUPLE_ELEM(1, elem)),                                    \
        BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, elem)),                                   \
        BOOST_PP_TUPLE_ELEM(1, elem)                                                        \
    );                                                                                      \
    static constexpr size_t offset = offsetof(data, BOOST_PP_TUPLE_ELEM(0,elem));           \
    BOOST_PP_IF(                                                                            \
        BOOST_PP_NOT(BOOST_PP_IS_EMPTY(BOOST_PP_TUPLE_ELEM(2, elem))),                      \
        static Type implicit_value() { return BOOST_PP_TUPLE_ELEM(2, elem); }               \
        ,                                                                                   \
    )                                                                                       \
};

// This helper macro transforms a field name into its corresponding Field_ struct name.
#define DETAILS_STRUCT_DESC_FIELD_NAME_OP(r, data, elem) \
    BOOST_PP_CAT(Field_, BOOST_PP_TUPLE_ELEM(0, elem))

// Define struct description
// Struct - type
// __VA_ARGS__ - field description tuple (field, opt_name, opt_impl_value, opt_validation)
#define STRUCT_DESC(Struct, ...)                                                                   \
namespace Slic3r::Biz::Yaml::Details {                                                             \
template <>                                                                                        \
struct StructTraits<Struct> {                                                                      \
    BOOST_PP_SEQ_FOR_EACH(DETAILS_STRUCT_DESC_FIELD, Struct, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))\
    using Fields = ::Slic3r::Biz::Yaml::Details::TypeList<                                         \
        BOOST_PP_SEQ_ENUM(                                                                         \
            BOOST_PP_SEQ_TRANSFORM(                                                                \
                DETAILS_STRUCT_DESC_FIELD_NAME_OP,                                                 \
                ~,                                                                                 \
                BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                                              \
            )                                                                                      \
        )                                                                                          \
    >;                                                                                             \
    using Type = Struct;                                                                           \
};                                                                                                 \
} // namespace  Yaml::Details

#define FIELD_DESC_SIMPLE(field) (field, #field,,)

#define DETAILS_TRANSFORM_FIELD_SIMPLE(r, data, elem) FIELD_DESC_SIMPLE(elem)
#define STRUCT_DESC_SIMPLE(Struct, ...)                                 \
    STRUCT_DESC(Struct,                                                 \
        BOOST_PP_SEQ_ENUM(                                              \
            BOOST_PP_SEQ_TRANSFORM(                                     \
                DETAILS_TRANSFORM_FIELD_SIMPLE,                         \
                ~,                                                      \
                BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                   \
            )                                                           \
        )                                                               \
    )

#define FIELD_DEFAULT
#define FIELD_NAME_SELF nullptr
#define FIELD_DESC(field, opt_field_name, opt_implicit_value, opt_validation) \
    (field, opt_field_name, opt_implicit_value, opt_validation)
#define FIELD_DESC_IMPLICIT_VALUE(field, implicit_value) \
        (field, FIELD_DEFAULT, implicit_value, FIELD_DEFAULT)

#define DETAILS_ENUM_VALUE_IF(r, data, elem)                \
    if (value == BOOST_PP_TUPLE_ELEM(0, elem))              \
        return data::BOOST_PP_TUPLE_ELEM(1, elem);
#define DETAILS_ENUM_VALUE_GET(r, data, elem)               \
    EnumValue(BOOST_PP_TUPLE_ELEM(0, elem), data::BOOST_PP_TUPLE_ELEM(1, elem))

#define ENUM_DESC(Enum, ...)                                                                    \
namespace Slic3r::Biz::Yaml::Details {                                                          \
template <>                                                                                     \
struct EnumTraits<Enum>                                                                         \
{                                                                                               \
    using Type = Enum;                                                                          \
    static constexpr std::optional<Type> parse(const std::string& value)                        \
    {                                                                                           \
        BOOST_PP_SEQ_FOR_EACH(                                                                  \
            DETAILS_ENUM_VALUE_IF,                                                              \
            Enum,                                                                               \
            BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                                               \
        )                                                                                       \
        return std::nullopt;                                                                    \
    }                                                                                           \
    using NameValuePair = EnumValue<Type>;                                                      \
    static constexpr std::array<NameValuePair, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)> values = {  \
        BOOST_PP_SEQ_ENUM(                                                                      \
            BOOST_PP_SEQ_TRANSFORM(                                                             \
                DETAILS_ENUM_VALUE_GET,                                                         \
                Enum,                                                                           \
                BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                                           \
            )                                                                                   \
        )                                                                                       \
    };                                                                                          \
};                                                                                              \
} //namespace Slic3r::Biz::Yaml::Details
