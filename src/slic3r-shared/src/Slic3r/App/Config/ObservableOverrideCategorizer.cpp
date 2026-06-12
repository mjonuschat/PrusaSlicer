///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ObservableOverrideCategorizer.hpp"

#include "Slic3r/Domain/Config.hpp"

namespace Slic3r::App {

bool ObservableOverrideCategorizer::allow_disabled() const
{
    return m_allow_disabled;
}

void ObservableOverrideCategorizer::set_allow_disabled(bool allow_disabled)
{
    if (m_allow_disabled != allow_disabled) {
        m_allow_disabled = allow_disabled;
        invalidate();
    }
}

ObservableOverrideCategorizer::ObservableOverrideCategorizer()
{
    set_filter_fn(
        [this](const Biz::OverrideItem& item) -> bool
        {
            if (!item.is_override()) {
                return false;
            }

            if (!m_allow_disabled && !item.overriden.value()) {
                return false;
            }

            return true;
        }
    );
    set_group_by_fn(
        [](const Biz::OverrideItem& item,
           std::unordered_set<Domain::ConfigItemDef::Category>& seen_keys) -> bool
        {
            Domain::ConfigItemDef::Category category = item.config_item->def().category;
            DEBUG_ASSERT(
                category != Domain::ConfigItemDef::Category::Unknown,
                "ConfigItemDef cannot have unknown category, please fill it."
            );

            if (seen_keys.contains(category)) {
                return true;
            } else {
                seen_keys.insert(category);
                return false;
            }
        }
    );
    set_sort_fn([](const Biz::OverrideItem& lhs, const Biz::OverrideItem& rhs)
                { return lhs.config_item->def().category < rhs.config_item->def().category; });
}

} // namespace Slic3r::App
