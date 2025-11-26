///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <cstddef>

namespace Slic3r::Domain {
class ConfigItem;
struct ConfigValue;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {

/**
 * @brief The IConfigBoxSetter class acts as Container for ConfigBoxInteractor
 */
class IConfigBoxSetter
{
public:
    IConfigBoxSetter()          = default;
    virtual ~IConfigBoxSetter() = default;

    virtual const Domain::ConfigValue*
    get_override_original_value(const Domain::ConfigItem& item, size_t index = 0) const = 0;

    virtual void set_item_value(
        const Domain::ConfigItem& item,
        const Domain::ConfigValue& value,
        size_t index = 0
    ) = 0;

    virtual void
    set_item_override(const Domain::ConfigItem& item, bool enable, size_t index = 0) = 0;
};

} // namespace Slic3r::Biz
