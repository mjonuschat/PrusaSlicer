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
    if (print_item->def().category == Domain::ConfigItemDef::Category::Hidden)
        return false;
    return false; // TODO
}
} // namespace Slic3r::Biz
