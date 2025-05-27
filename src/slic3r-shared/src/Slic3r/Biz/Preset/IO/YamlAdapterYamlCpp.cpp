#include <boost/iostreams/stream.hpp>
#include "YamlAdapterYamlCpp.hpp"

namespace Yaml::YamlCpp {

YamlAdapterYamlCpp::Parser YamlAdapterYamlCpp::create_file_parser(const char* file_name)
{
    return {
        .nodes = YAML::LoadAllFromFile(file_name),
        .file=file_name
    };
}

YamlAdapterYamlCpp::Parser YamlAdapterYamlCpp::create_string_parser(std::string_view data)
{
    boost::iostreams::stream<boost::iostreams::array_source> stream{data.data(), data.size()};
    return {
        .nodes = YAML::LoadAll(stream),
        .file="<string>"
    };
}

YamlAdapterYamlCpp::Document YamlAdapterYamlCpp::load(const Parser& parser)
{
    if (parser.current < parser.nodes.size()) {
        const auto node = parser.nodes[parser.current++];
        return {.node = node, .file = parser.file};
    }
    return {.node=std::nullopt, .file=parser.file};
}

Yaml::Details::NodeType YamlAdapterYamlCpp::node_type(const NodeRef& node)
{
    auto type = node.node->Type();
    switch (type) {
    case YAML::NodeType::Map:
        return Yaml::Details::NodeType::Mapping;
    case YAML::NodeType::Sequence:
        return Yaml::Details::NodeType::Sequence;
    default:
        return Yaml::Details::NodeType::Scalar;
    }
}

std::string_view YamlAdapterYamlCpp::scalar_value(const NodeRef& node)
{
    return node.node->Scalar();
}

size_t YamlAdapterYamlCpp::sequence_item_count(const NodeRef& node)
{
    return node.node->size();
}

YamlAdapterYamlCpp::NodeRef YamlAdapterYamlCpp::sequence_item_at(const NodeRef& node, size_t index)
{
    return {.node=(*node.node)[index], .file=node.file};
}

size_t YamlAdapterYamlCpp::mapping_item_count(const NodeRef& node)
{
    return node.node->size();
}

YamlAdapterYamlCpp::NodeRef YamlAdapterYamlCpp::mapping_value_at(const NodeRef& node, std::string_view name)
{
    try {
#if defined(_MSC_VER)
        // For some reason MSVC has problems with string_view, lets allocate string for it
        std::string key{name};
#else
        auto key = name;
#endif
        return {.node=(*node.node)[key], .file=node.file};
    }
    catch (YAML::KeyNotFound& e) {
        return {.node = std::nullopt, .file = node.file};
    }
}

YamlAdapterYamlCpp::KeyValuePair YamlAdapterYamlCpp::mapping_key_value_at(const NodeRef& node, size_t index)
{
    auto it = node.node->begin();
    std::advance(it, index);
    return it;
}

YamlAdapterYamlCpp::NodeRef YamlAdapterYamlCpp::key(const KeyValuePair& pair, const NodeRef& parent)
{
    return {.node=pair->first, .file=parent.file};
}

YamlAdapterYamlCpp::NodeRef YamlAdapterYamlCpp::value(const KeyValuePair& pair, const NodeRef& parent)
{
    return {.node=pair->second, .file=parent.file};
}

Yaml::Details::Mark YamlAdapterYamlCpp::mark(const NodeRef& node)
{
    if (!node.node.has_value())
        return {.file=node.file};
    auto mark = node.node->Mark();
    return {.file=node.file, .line=size_t(mark.line + 1), .column=size_t(mark.column + 1)};
}
}
