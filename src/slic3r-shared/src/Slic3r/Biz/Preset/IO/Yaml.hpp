#pragma once

#include <stdexcept>
#include <cstddef>
#include <map>
#include <memory>
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

#if defined(SLIC3R_YAML_LIBFYAML)
#include "YamlAdapterLibfyaml.hpp"
#elif defined(SLIC3R_YAML_YAMLCPP)
#include "YamlAdapterYamlCpp.hpp"
#else
#error "No YAML library selected
#endif

namespace Yaml {

#if defined(SLIC3R_YAML_LIBFYAML)
using YamlAdapter = Libfyaml::YamlAdapterLibfyaml;
#elif defined(SLIC3R_YAML_YAMLCPP)
using YamlAdapter = YamlCpp::YamlAdapterYamlCpp;
#else
#error "No YAML library selected
#endif



YamlAdapter::Document parse_file(const char* file_name);
YamlAdapter::Document parse_string(std::string_view yaml);

void parse_all_documents_in_file(const char* file_name, const std::function<void(const YamlAdapter::Document&)>& parse_doc);
void parse_all_documents_in_string(std::string_view yaml, const std::function<void(const YamlAdapter::Document&)>& parse_doc);

struct ParseError : std::runtime_error
{
    ParseError(const YamlAdapter::NodeRef& node, const std::string& msg)
        : std::runtime_error(describe(node) + ": " + msg)
    {}

    ParseError(const ParseError&) = default;
    ParseError(ParseError&&) = default;

    ParseError& operator=(const ParseError&) = default;
    ParseError& operator=(ParseError&&) = default;

    ParseError decorate_with_source(const std::string& source) const
    {
        return ParseError("[" + source + "] " + what());
    }

private:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
    explicit ParseError(const char* msg) : std::runtime_error(msg) {}

    static std::string describe(const YamlAdapter::NodeRef& node)
    {
        auto mark = YamlAdapter::mark(node);
        return fmt::format("[file: {}:{}:{}]", node.file, mark.line, mark.column);
    }
};

/**
 * @}
 */


namespace Details {
template <typename T>
struct StructTraits{};
}

template <typename T>
typename Details::StructTraits<T>::Type parse_struct(const YamlAdapter::NodeRef& node);

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

inline void ensure_node_not_null(const YamlAdapter::NodeRef& node)
{
    if (node.is_null())
        throw ParseError(
            node,
            "Null is not allowed here"
        );
}

inline void ensure_node_type(const YamlAdapter::NodeRef& node, NodeType type)
{
    auto node_type = YamlAdapter::node_type(node);
    if (node_type != type)
        throw ParseError(
            node,
            std::string("Node type mismatch, expecting '") + node_type_value(type) +
                "' but got '" + node_type_value(node_type) + "'"
        );
}

inline std::string_view get_node_scalar(const YamlAdapter::NodeRef& node)
{
    ensure_node_type(node, NodeType::Scalar);
    ensure_node_not_null(node);
    return YamlAdapter::scalar_value(node);
}

template <typename T, typename Enabled = void>
struct TypeTraits {};

template <>
struct TypeTraits<bool>
{
    static bool parse(const YamlAdapter::NodeRef& node)
    {
        auto value = get_node_scalar(node);
        if (value == "true")
            return true;
        if (value == "false")
            return false;

        throw ParseError(node, fmt::format("Invalid bool value: '{}', allowed values are 'true' and 'false'", value));
    }
};

template <typename T, typename P>
T parse_with_spirit(const YamlAdapter::NodeRef& node, P parser)
{
    auto value = get_node_scalar(node);
    T ret;
    namespace qi = boost::spirit::qi;
    auto it = std::cbegin(value);
    if (!qi::parse(it, std::cend(value), parser, ret) || it != std::cend(value))
        throw ParseError(node, fmt::format("Invalid {} value: '{}'", typeid(T).name(),value));
    return ret;
}


#define TYPE_TRAITS_WITH_SPIRIT_PARSE(T, P)         \
template <>                                         \
struct TypeTraits<T>                                \
{                                                   \
    static T parse(const YamlAdapter::NodeRef& node)             \
    {                                               \
        return parse_with_spirit<T>(node, P);       \
    }                                               \
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
    static std::string parse(const YamlAdapter::NodeRef& node)
    {
        auto value = get_node_scalar(node);
        return std::string{value};
    }
};

template <typename, typename = void>
struct HasTypeTraits : std::false_type {};

template <typename T>
struct HasTypeTraits<T, std::void_t<decltype(TypeTraits<T>::parse(std::declval<const YamlAdapter::NodeRef&>()))>> : std::true_type {};


template <typename T>
struct TypeTraits<std::optional<T>, std::enable_if_t<HasTypeTraits<T>::value>>
{
    static std::optional<T> parse(const YamlAdapter::NodeRef& node)
    {
        if (node && !node.is_null())
            return TypeTraits<T>::parse(node);
        return std::nullopt;
    }
};

template <typename T>
struct TypeTraits<std::vector<T>, std::enable_if_t<HasTypeTraits<T>::value>>
{
    static std::vector<T> parse(const YamlAdapter::NodeRef& node)
    {
        ensure_node_type(node, NodeType::Sequence);
        const size_t n = YamlAdapter::sequence_item_count(node);
        std::vector<T> ret;
        ret.reserve(n);
        for (size_t i = 0; i < n; ++i)
            ret.push_back(TypeTraits<T>::parse(YamlAdapter::sequence_item_at(node, i)));
        return ret;
    }
};

template <typename K, typename V>
struct TypeTraits<std::map<K, V>, std::enable_if_t<HasTypeTraits<K>::value && HasTypeTraits<V>::value>>
{
    static std::map<K, V> parse(const YamlAdapter::NodeRef& node)
    {
        ensure_node_type(node, NodeType::Mapping);
        const size_t n = YamlAdapter::mapping_item_count(node);
        std::map<K, V> ret;
        for (size_t i = 0; i < n; ++i) {
            auto kv_pair = YamlAdapter::mapping_key_value_at(node, i);
            auto key_node = YamlAdapter::key(kv_pair, node);
            auto value_node = YamlAdapter::value(kv_pair, node);;
            K key = TypeTraits<K>::parse(key_node);
            V value = TypeTraits<V>::parse(value_node);
            ret.emplace(std::move(key), std::move(value));
        }
        return ret;
    }
};


template <typename V, typename T>
V parse_variant(const YamlAdapter::NodeRef& node)
{
    return TypeTraits<T>::parse(node);
}

template <typename V, typename T, typename ...Ts, std::enable_if_t<(sizeof...(Ts) > 0), int> = 0>
V parse_variant(const YamlAdapter::NodeRef& node)
{
    try {
        return TypeTraits<T>::parse(node);
    } catch (const ParseError&) {
        return parse_variant<V, Ts...>(node);
    }
}

template <typename ... Ts>
struct AllHasTypeTraits : std::bool_constant<(HasTypeTraits<Ts>::value && ...)> {};

template <typename ...Ts>
struct TypeTraits<
    std::variant<Ts...>,
    std::enable_if_t<AllHasTypeTraits<Ts...>::value>
>
{
    using ValueType = std::variant<Ts...>;
    static ValueType parse(const YamlAdapter::NodeRef& node)
    {
        return parse_variant<ValueType, Ts...>(node);
    }
};

template <>
struct TypeTraits<std::monostate>
{
    using ValueType = std::monostate;
    static ValueType parse(const YamlAdapter::NodeRef& node)
    {
        if (node.is_null())
            return {};
        throw ParseError(node, "Node must be null");
    }
};

template <typename F, typename = void>
struct FieldHasImplicitValue : std::false_type {};

template <typename F>
struct FieldHasImplicitValue<F, std::void_t<decltype(F::implicit_value())>> : std::true_type {};

template <typename T>
struct IsOptional : std::false_type {};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type {};

template <typename F>
struct FieldIsOptional : IsOptional<typename F::Type> {};

template <typename F, typename = void>
struct FieldHasValidation : std::false_type {};

template <typename F>
struct FieldHasValidation<F, std::void_t<decltype(F::validate(std::declval<typename F::Type>()))>> : std::true_type {};


template <typename ... Ts>
struct TypeList {};


template <typename Field, std::enable_if_t<!FieldHasImplicitValue<Field>::value, int> = 0>
void parse_field(typename Field::Type& dest, const YamlAdapter::NodeRef& node)
{
    dest = TypeTraits<typename Field::Type>::parse(node);
}

template <typename Field, std::enable_if_t<FieldHasImplicitValue<Field>::value, int> = 0>
void parse_field(typename Field::Type& dest, const YamlAdapter::NodeRef& node)
{
    if (!node)
        dest = Field::implicit_value();
    else
        dest = TypeTraits<typename Field::Type>::parse(node);
}

template <typename Field, typename = void>
void validate_field(const typename Field::Type& type)
{}

template <typename Field, std::enable_if_t<FieldHasValidation<Field>::value>>
void validate_field(const typename Field::Type& type)
{
    Field::validate(type);
}


template <typename S, typename Field, typename = void>
void parse_field(S& s, const YamlAdapter::NodeRef& node)
{
    using FT = typename Field::Type;
    auto* raw_storage = reinterpret_cast<char*>(&s) + Field::offset;
    FT& typed_storage = *reinterpret_cast<FT*>(raw_storage);
    parse_field<Field>(typed_storage, node);
    validate_field<Field>(typed_storage);
}


template <typename Field, std::enable_if_t<
    !(FieldHasImplicitValue<Field>::value || FieldIsOptional<Field>::value),
int> = 0>
YamlAdapter::NodeRef get_mapping_node_with_key(const YamlAdapter::NodeRef& node, const char* key)
{
    auto value_node = YamlAdapter::mapping_value_at(node, key);
    if (!value_node)
        throw ParseError(node, fmt::format("Required field '{}' not found", key));
    return value_node;
}

template <typename Field, std::enable_if_t<
    FieldHasImplicitValue<Field>::value || FieldIsOptional<Field>::value,
int> = 0>
YamlAdapter::NodeRef get_mapping_node_with_key(const YamlAdapter::NodeRef& node, const char* key)
{
    return YamlAdapter::mapping_value_at(node, key);
}

template <typename T, typename F>
struct ParseFieldTypeList;

template <typename S, typename ... Fs>
struct ParseFieldTypeList<S, TypeList<Fs...>>
{
    static void parse(S& s, const YamlAdapter::NodeRef& node)
    {
        (parse_field<S, Fs>(s, Fs::name == nullptr ? node : get_mapping_node_with_key<Fs>(node, Fs::name)), ...);
    }
};

template <typename S>
typename S::Type parse_struct_helper(const YamlAdapter::NodeRef& node)
{
    typename S::Type ret;

    ParseFieldTypeList<
        typename S::Type,
        typename S::Fields
    >::parse(ret, node);

    return ret;
}

template <typename T>
bool try_parse_discriminated_struct(const YamlAdapter::NodeRef& node, std::string_view value, std::tuple<const char*, std::function<void(T&&)>> loader)
{
    if (value == std::get<0>(loader)) {
        T s = parse_struct<T>(node);
        std::get<1>(loader)(std::move(s));
        return true;
    }
    return false;
}


template <typename, typename = void>
struct HasStructTraits : std::false_type {};

template <typename T>
struct HasStructTraits<T, std::void_t<typename StructTraits<T>::Type>> : std::true_type {};

template <typename T>
struct TypeTraits<T, std::enable_if_t<HasStructTraits<T>::value>>
{
    static T parse(const YamlAdapter::NodeRef& node)
    {
        return parse_struct<T>(node);
    }
};

template <typename E, typename = void>
struct EnumTraits {};

template <typename E>
struct EnumValue
{
    const char* name;
    const E value;

    constexpr EnumValue(const char* name, E value) noexcept : name(name), value(value) {}
};


template <typename, typename = void>
struct HasEnumTraits : std::false_type {};

template <typename T>
struct HasEnumTraits<T, std::void_t<typename EnumTraits<T>::Type>> : std::true_type {};

template <typename T>
struct TypeTraits<T, std::enable_if_t<std::is_enum_v<T> && !HasEnumTraits<T>::value>>
{
    static T parse(const YamlAdapter::NodeRef& node)
    {
        auto value = get_node_scalar(node);
        auto ret = magic_enum::enum_cast<T>(value, magic_enum::case_insensitive);
        if (!ret.has_value()) {
            auto allowed_values = magic_enum::enum_names<T>();
            throw ParseError(node, fmt::format(
                "Invalid enum value: '{}', allowed values: {}",
                value,
                fmt::join(allowed_values.begin(), allowed_values.end(), ", ")
            ));
        }
        return ret.value();
    }
};


template <typename T>
struct TypeTraits<T, std::enable_if_t<HasEnumTraits<T>::value>>
{
    static T parse(const YamlAdapter::NodeRef& node)
    {
        auto value = TypeTraits<std::string>::parse(node);
        const auto& values = EnumTraits<T>::values;
        auto it = std::find_if(values.begin(), values.end(), [&](const auto& v) {
            return v.name == value;
        });
        if (it == values.end()) {
            auto keys = values | std::views::transform([](const EnumValue<T>& ev) -> const char* {
                return ev.name;
            });
            throw ParseError(node, fmt::format(
                "Invalid enum value: '{}', allowed values: {}",
                value,
                fmt::join(keys, ",")
            ));
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
 * @brief Parse structure from libfyaml node
 * @tparam T Type of structure. Note that this structure needs STRUCT_DESC(...) definition.
 * @param node Yaml node representing the structure to load.
 * @return Loaded structure or ParseError exception is thrown.
 */
template <typename T>
typename Details::StructTraits<T>::Type parse_struct(const YamlAdapter::NodeRef& node)
{
    return Details::parse_struct_helper<Details::StructTraits<T>>(node);
}

/**
 * @brief Parse structure from libfyaml node
 * @tparam T Type of structure. Note that this structure needs STRUCT_DESC(...) definition.
 * @param doc Yaml YamlAdapter::Document containing the structure to load.
 * @return Loaded structure or ParseError exception is thrown.
 */
template <typename T>
typename Details::StructTraits<T>::Type parse_struct(const YamlAdapter::Document& doc)
{
    return parse_struct<T>(doc.root());
}

/**
 * @brief Load struct from possible set of types distinguished by value in discriminator field.
 * @tparam Ts types of structs to load
 * @param node Yaml node containing structure to load and the discriminator_field_name
 * @param discriminator_field_name A name of field the type of structure is distinguished by
 * @param loaders List (variadic args) of two element tuples containing `const char*` value of
 * discriminator and function that will be called with loaded struct of given type.
 */
template <typename ... Ts>
void parse_structs_by_discriminant(
    const YamlAdapter::NodeRef& node, const char* discriminator_field_name,
    const std::tuple<const char*, std::function<void(Ts&&)>>& ... loaders
)
{
    auto discr_node = YamlAdapter::mapping_value_at(node, discriminator_field_name);
    auto discr_value = Details::get_node_scalar(discr_node);
    if (!(Details::try_parse_discriminated_struct(node, discr_value, loaders) || ...)) {
        std::vector<const char*> discr_field_values;
        (discr_field_values.push_back(std::get<0>(loaders)), ...);
        throw ParseError(node, fmt::format(
            "Cannot parse any structure identified by field (discriminator) '{}' with value '{}'. Allowed values are: {}",
            discriminator_field_name, discr_value,
            fmt::join(discr_field_values.begin(), discr_field_values.end(), ", ")
        ));
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
template <typename ... Ts>
void parse_structs_by_discriminant(
    const YamlAdapter::Document& doc, const char* discriminator_field_name,
    const std::tuple<const char*, std::function<void(Ts&&)>>& ... loaders
)
{
    parse_discriminated_structs(
        doc.root(),
        discriminator_field_name,
        loaders...
    );
}

} // namespace Yaml




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
namespace Yaml::Details {                                                                          \
template <>                                                                                        \
struct StructTraits<Struct> {                                                                      \
    BOOST_PP_SEQ_FOR_EACH(DETAILS_STRUCT_DESC_FIELD, Struct, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))\
    using Fields = ::Yaml::Details::TypeList<                                                      \
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


#define DETAILS_ENUM_VALUE_IF(r, data, elem)                \
    if (value == BOOST_PP_TUPLE_ELEM(0, elem))              \
        return data::BOOST_PP_TUPLE_ELEM(1, elem);
#define DETAILS_ENUM_VALUE_GET(r, data, elem)               \
    EnumValue(BOOST_PP_TUPLE_ELEM(0, elem), data::BOOST_PP_TUPLE_ELEM(1, elem))



#define ENUM_DESC(Enum, ...)                                                                    \
namespace Yaml::Details {                                                                       \
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
} //namespace Yaml::Details


