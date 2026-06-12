///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ObservableCategorizer.hpp"

namespace Slic3r::App {

ObservableCategorizer::ObservableCategorizer()
{
    set_filter_fn([](const Domain::ConfigItem& item) {
        return item.def().category != Domain::ConfigItemDef::Category::Hidden;
    });
    set_group_by_fn(
        [](const Domain::ConfigItem& item,
           std::unordered_set<Domain::ConfigItemDef::Category>& seen_keys) -> bool {
        DEBUG_ASSERT(
            item.def().category != Domain::ConfigItemDef::Category::Unknown,
            "ConfigItemDef cannot have unknown category, please fill it."
        );

        if (seen_keys.contains(item.def().category)) {
            return true;
        } else {
            seen_keys.insert(item.def().category);
            return false;
        }
    }
    );
    set_sort_fn([](const Domain::ConfigItem& lhs, const Domain::ConfigItem& rhs) -> int {
        return static_cast<uint8_t>(lhs.def().category) < static_cast<uint8_t>(rhs.def().category);
    });
}

} // namespace Slic3r::App
