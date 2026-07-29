///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/PrintToolItem.hpp"

#include "Slic3r/Domain/Config.hpp"

namespace Slic3r::Biz {

const Domain::ConfigValue& PrintToolItem::tool_value(size_t index) const
{
    ASSERT(index < tool_overrides.size());
    return tool_overrides.at(index)->value();
}

void PrintToolItem::update_value()
{
    value = Domain::apply_compatibility_rule(
        &print_item->value(),
        tool_overrides,
        shared_context.extruder_candidates
    );
}

bool PrintToolItem::is_dirty() const
{
    if (print_item->def().category == Domain::ConfigItemDef::Category::Hidden) {
        return false;
    }
    // With multiple tools (>1), each tool can override this value independently, so the
    // print-level value is not user-facing here — only tool-level overrides can be dirty.
    // With a single tool there is no meaningful distinction between print and tool value,
    // so only the print-level dirtiness is reported.
    if (tool_overrides.size() > 1) {
        return is_dirty_tool();
    }
    return is_dirty_print();
}

bool PrintToolItem::is_dirty_print() const
{
    return !original_print_item || original_print_item->value() != print_item->value();
}

bool PrintToolItem::is_dirty_tool(std::optional<size_t> index) const
{
    if (index.has_value()) {
        return index.value() < original_tool_overrides.size()
            && original_tool_overrides.at(index.value())->value() != tool_value(index.value());
    }
    for (size_t tool_id{}; tool_id < tool_overrides.size(); tool_id++) {
        if (is_dirty_tool(tool_id)) {
            return true;
        }
    }
    return false;
}

} // namespace Slic3r::Biz
