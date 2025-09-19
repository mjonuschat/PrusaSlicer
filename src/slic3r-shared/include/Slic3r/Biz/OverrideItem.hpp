///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <string>
#include <optional>

namespace Slic3r::Domain {
struct ConfigBox;
class ConfigItem;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {
/**
 * @brief The OverrideItem class is a wrapper for Domain::ConfigItem from multiple sources
 */
struct OverrideItem
{
    std::string name;
    bool mixed = false; ///< true if all sources values are same, false otherwise
    std::optional<bool> overriden; ///< true if overide is on, false if override is off, nullopt if item is not override
    const Domain::ConfigItem* config_item = nullptr; ///< pointer to this item in the first source

    inline bool is_override() const
    {
        return overriden.has_value();
    }
};

} // namespace Slic3r::Biz
