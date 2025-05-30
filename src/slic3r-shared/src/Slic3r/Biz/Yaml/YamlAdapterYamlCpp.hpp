#pragma once
#include <string>
#include <optional>
#include <yaml-cpp/yaml.h>

#include "Slic3r/Biz/Yaml/YamlAdapter.hpp"


namespace Slic3r::Biz::Yaml::YamlCpp {
namespace Details {
struct NodeRef
{
    std::optional<YAML::Node> node;
    std::string file;

    operator bool() const
    {
        return node.has_value() && node->IsDefined();
    }

    bool is_null() const { return node.has_value() && node->IsNull(); }
};

struct Parser
{
    std::vector<YAML::Node> nodes;
    mutable size_t current{0};
    std::string file;
};

struct Document
{
    std::optional<YAML::Node> node;
    std::string file;

    operator bool() const { return node.has_value(); }
    NodeRef root() const { return {.node=node.value(), .file=file}; }
};

using KeyValuePair = YAML::const_iterator;

struct ConstIterator
{
    const NodeRef& node;
    YAML::Node::const_iterator it;

    ConstIterator begin() const { return {.node=node, .it=node.node->begin()}; }
    ConstIterator end() const { return {.node=node, .it=node.node->end()}; }

    bool operator==(const ConstIterator& rhs) const { return it == rhs.it; }
    bool operator!=(const ConstIterator& rhs) const { return it != rhs.it; }

    ConstIterator& operator++() { it++; return *this; }
    ConstIterator operator++(int)
    {
        auto ret = *this;
        ++(*this);
        return ret;
    }
};

} // namespace Details

struct YamlAdapterYamlCpp
{
    using Parser = Details::Parser;
    using Document = Details::Document;
    using NodeRef = Details::NodeRef;
    using KeyValuePair = Details::KeyValuePair;


    static Parser create_file_parser(const char* file_name);
    static Parser create_string_parser(std::string_view data);

    static Document load(const Parser& parser);

    static Yaml::Details::NodeType node_type(const NodeRef& node);

    static std::string_view scalar_value(const NodeRef& node);

    static size_t sequence_item_count(const NodeRef& node);
    static NodeRef sequence_item_at(const NodeRef& node, size_t index);

    static size_t mapping_item_count(const NodeRef& node);
    static NodeRef mapping_value_at(const NodeRef& node, std::string_view name);
    static KeyValuePair mapping_key_value_at(const NodeRef& node, size_t index);
    static NodeRef key(const KeyValuePair& pair, const NodeRef& parent);
    static NodeRef value(const KeyValuePair& pair, const NodeRef& parent);

    static Yaml::Details::Mark mark(const NodeRef& node);

};

static_assert(Yaml::Details::YamlAdapterInterface<YamlAdapterYamlCpp>);

}

