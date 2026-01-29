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
    const ToolOverride& override = tool_overrides.at(index);
    return override.second ? override.first->value() : print_item->value();
}

void PrintToolItem::update_value()
{
    value = Domain::apply_compatibility_rule(
        &print_item->value(),
        construct_overrides(),
        extruder_candidates
    );
}

std::vector<const Domain::ConfigItem*> PrintToolItem::construct_overrides() const
{
    std::vector<const Domain::ConfigItem*> overrides;
    overrides.reserve(tool_overrides.size());

    std::transform(
        tool_overrides.cbegin(),
        tool_overrides.cend(),
        std::back_inserter(overrides),
        [](const ToolOverride& tool_override)
        { return tool_override.second ? tool_override.first : nullptr; }
    );

    return overrides;
}

} // namespace Slic3r::Biz
