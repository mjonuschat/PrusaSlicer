///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/ConfigBoxObservableList.hpp"

namespace Slic3r::Biz {

void ConfigBoxObservableList::set_config_box(Domain::ConfigBox* config_box)
{
    // if (m_config_box != config_box) {
    invoke_listeners<IListObserver<Domain::ConfigItem>>(
        [&](IListObserver<Domain::ConfigItem>* l)
        { l->on_will_be_reset(); }
    );
    m_config_box = config_box;
    invoke_listeners<IListObserver<Domain::ConfigItem>>([&](IListObserver<Domain::ConfigItem>* l)
                                                        { l->on_reset(); });
    // }
}

const Domain::ConfigItem& ConfigBoxObservableList::at(size_t index) const
{
    return m_config_box->items.all_items().at(index);
}

size_t ConfigBoxObservableList::size() const
{
    return m_config_box->items.all_items().size();
}

void ConfigBoxObservableList::set_value(const std::string_view key, const Domain::ConfigValue& value)
{
    const std::vector<Domain::ConfigItem>& all_items = m_config_box->items.all_items();

    const std::vector<Domain::ConfigItem>::const_iterator index_it = std::find_if(
        all_items.cbegin(),
        all_items.cend(),
        [key](const Domain::ConfigItem& item) { return item.def().name == key; }
    );

    if (index_it != all_items.cend() && index_it->value() != value) {
        const size_t index = std::distance(all_items.cbegin(), index_it);

        m_config_box->items.opt(key).set(value);

        invoke_listeners<IListObserver<Domain::ConfigItem>>(
            [index](IListObserver<Domain::ConfigItem>* l) { l->on_updated(index); }
        );
    }
}

const Domain::ConfigValue* ConfigBoxObservableList::find(const std::string& name) const
{
    Domain::ConfigItem* found_item = m_config_box->items.find(name);
    return found_item ? &found_item->value() : nullptr;
}

void ConfigBoxOverridesObservableList::set_config_box(Domain::ConfigBox* config_box)
{
    // if (m_config_box != config_box)
    invoke_listeners<IListObserver<Domain::ConfigItem>>(
        [&](IListObserver<Domain::ConfigItem>* l)
        { l->on_will_be_reset(); }
    );
    m_config_box = config_box;
    invoke_listeners<IListObserver<Domain::ConfigItem>>([&](IListObserver<Domain::ConfigItem>* l)
                                                        { l->on_reset(); });
    // }
}

const Domain::ConfigItem& ConfigBoxOverridesObservableList::at(size_t index) const
{
    return m_config_box->overrides.all_items().at(index);
}

size_t ConfigBoxOverridesObservableList::size() const
{
    return m_config_box->overrides.all_items().size();
}

void ConfigBoxOverridesObservableList::set_value(const std::string& key, const Domain::ConfigValue& value)
{
    const std::vector<Domain::ConfigItem>& all_items = m_config_box->overrides.all_items();

    std::vector<Domain::ConfigItem>::const_iterator index = std::find_if(
        all_items.cbegin(),
        all_items.cend(),
        [key](const Domain::ConfigItem& item) { return item.def().name == key; }
    );

    if (index != all_items.cend() && index->value() != value) {
        m_config_box->overrides.set(key, value);

        invoke_listeners<IListObserver<Domain::ConfigItem>>([&](IListObserver<Domain::ConfigItem>* l) {
            l->on_updated(std::distance(all_items.cbegin(), index));
        });
    }
}

void ConfigBoxOverridesObservableList::set_override(const std::string& key, bool enable)
{
    const std::vector<Domain::ConfigItem>& all_items      = m_config_box->overrides.all_items();
    std::vector<Domain::ConfigItem>::const_iterator index = std::find_if(
        all_items.cbegin(),
        all_items.cend(),
        [key](const Domain::ConfigItem& item) { return item.def().name == key; }
    );

    if (index != all_items.cend()) {
        if (enable) {
            m_config_box->overrides.enable(key);
        } else {
            m_config_box->overrides.disable(key);
        }

        invoke_listeners<IListObserver<Domain::ConfigItem>>([&](IListObserver<Domain::ConfigItem>* l) {
            l->on_updated(std::distance(all_items.cbegin(), index));
        });
    }
}

} // namespace Slic3r::Biz
