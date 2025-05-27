#pragma once

#include <concepts>
#include <type_traits>
#include <string>
#include <string_view>

namespace Yaml::Details {
enum class NodeType
{
    Scalar, Sequence, Mapping
};

struct Mark
{
    std::string file;
    size_t line{0};
    size_t column{0};
};

template <typename T, typename NodeRefT>
concept DocumentInterface = requires {
    { std::declval<T>() } -> std::convertible_to<bool>;
    { std::declval<T>().root() } -> std::same_as<NodeRefT>;
};

template <typename T>
concept NodeRefInterface = requires {
    { std::declval<T>() } -> std::convertible_to<bool>;
    { std::declval<const T>().is_null() } -> std::same_as<bool>;
};

template<typename T>
concept YamlAdapterInterface = requires(
    const char* filename,
    std::string_view yaml,
    const typename T::Parser& parser,
    const typename T::NodeRef& node,
    size_t index,
    std::string_view name,
    const typename T::KeyValuePair& pair
)
{
    typename T::NodeRef;
    typename T::Document;
    typename T::Parser;
    typename T::KeyValuePair;

    { T::create_file_parser(filename) } -> std::same_as<typename T::Parser>;
    { T::create_string_parser(yaml) } -> std::same_as<typename T::Parser>;
    { T::load(parser) } -> std::same_as<typename T::Document>;
    { T::node_type(node) } -> std::same_as<NodeType>;
    { T::scalar_value(node) } -> std::same_as<std::string_view>;
    { T::sequence_item_count(node) } -> std::same_as<size_t>;
    { T::sequence_item_at(node, index) } -> std::same_as<typename T::NodeRef>;
    { T::mapping_item_count(node) } -> std::same_as<size_t>;
    { T::mapping_value_at(node, name) } -> std::same_as<typename T::NodeRef>;
    { T::mapping_key_value_at(node, index) } -> std::same_as<typename T::KeyValuePair>;
    { T::key(pair, node) } -> std::same_as<typename T::NodeRef>;
    { T::value(pair, node) } -> std::same_as<typename T::NodeRef>;
    { T::mark(node) } -> std::same_as<Mark>;
} && DocumentInterface<typename T::Document, typename T::NodeRef> && NodeRefInterface<typename T::NodeRef>;

}
