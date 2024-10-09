#pragma once

#include <stack>
#include "Slic3r/App/Scene/NodeVisitorTypes.hpp"
#include "Slic3r/App/Scene/Node.hpp"

namespace Slic3r::App::Scene {

void visit(const Node& node, const ConstNodeVisitor& visitor, bool ignore_enabled = false);
void visit(Node& node, const NodeVisitor& visitor, bool ignore_enabled = false);
void visit_conditional(const Node& node, const ConstNodeConditionalVisitor& visitor, bool ignore_enabled = false);
void visit_conditional(Node& node, const NodeConditionalVisitor& visitor, bool ignore_enabled = false);

template<typename Result>
std::vector<std::pair<const Node*, Result>> visit_conditional_transform(
    const Node& node, const ConstNodeConditionalTransform<Result>& transform,
    bool ignore_enabled = false
)
{
    std::vector<std::pair<const Node*, Result>> ret;
    visit(node, [&ret, transform](const Node& n) {
        Result result;
        if (transform(n, result))
            ret.emplace_back(&n, result);
    }, ignore_enabled);
    return ret;
}

template<typename Result>
std::vector<std::pair<Node*, Result>> visit_conditional_transform(
    Node& node, const NodeConditionalTransform<Result>& transform,
    bool ignore_enabled = false
)
{
    std::vector<std::pair<Node*, Result>> ret;
    visit(node, [&ret, transform](Node& n) {
        Result result;
        if (transform(n, result))
            ret.emplace_back(&n, result);
    }, ignore_enabled);
    return ret;
}

}
