///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Domain/ConfigValue.hpp>

#include <string>
#include <set>

namespace Slic3r::Domain {
struct ConfigBox;
class ConfigItem;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {
/**
 * @brief The ToolPrintItem class is a wrapper for Domain::ConfigItem for Items
 * defined in Print and tool. Essentially these two sources are aggregated
 * into a single ToolPrintItem.
 */
struct PrintToolItem
{
    struct SharedContext
    {
        bool has_multiple_extruders = false;
        const std::set<unsigned>&
            extruder_candidates; ///< extruder candidates vector stored probably in PrintToolCBOL
    };

    std::string name;
    bool mixed = false; ///< true if all sources values are same, false otherwise
    const Domain::ConfigItem* print_item{nullptr}; ///< pointer to this item in the Print ConfigBox
    std::vector<const Domain::ConfigItem*> tool_overrides; ///< vector of turned overrides from Tool
    std::pair<Domain::ConfigValue, bool> value;
    const SharedContext& shared_context;
    bool is_favorite{false};

    const Domain::ConfigValue& tool_value(size_t index) const;

    void update_value();

    bool is_dirty() const;
};

} // namespace Slic3r::Biz
