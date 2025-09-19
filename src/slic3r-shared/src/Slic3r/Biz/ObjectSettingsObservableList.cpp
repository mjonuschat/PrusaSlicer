///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/Biz/ObjectSettingsObservableList.hpp"

namespace Slic3r::Biz {

const OverrideItem& ObjectSettingsObservableList::at(size_t index) const
{
    ASSERT(index < m_items.size());

    return *m_items.at(index).get();
}

size_t ObjectSettingsObservableList::size() const
{
    return m_items.size();
}

void ObjectSettingsObservableList::set_sources(const std::vector<Domain::ConfigBox*>& sources)
{
    invoke_listeners<IListObserver<OverrideItem>>([&](IListObserver<OverrideItem>* l) {
        l->on_will_be_reset();
    });

    m_sources = sources;

    m_items.clear();
    m_item_index.clear();
    m_item_sources.clear();

    // 1. Go through all Config Boxes and populate m_item_sources, m_items and m_item_index
    for (Domain::ConfigBox* box : sources) {
        for (const Domain::ConfigItem& config_item : box->items.all_items()) {
            OverrideItem* item = find_item(config_item.name());
            if (!item) {
                OverrideItemPtr& item_ptr = m_items.emplace_back(
                    std::make_unique<OverrideItem>(config_item.name(), false, std::optional<bool>(), &config_item)
                );
                m_item_sources[item_ptr.get()].insert(box);
                m_item_index.insert({config_item.name(), m_items.size() - 1});
            } else {
                m_item_sources[item].insert(box);
            }
        }

        for (const Domain::ConfigItem& config_item : box->overrides.all_items()) {
            OverrideItem* item = find_item(config_item.name());
            if (!item) {
                OverrideItemPtr& item_ptr = m_items.emplace_back(
                    std::make_unique<OverrideItem>(
                        config_item.name(),
                        false,
                        box->overrides.get(config_item.name()).has_value(),
                        &config_item
                    )
                );
                m_item_sources[item_ptr.get()].insert(box);
                m_item_index.insert({config_item.name(), m_items.size() - 1});
            } else {
                m_item_sources[item].insert(box);
                item->overriden = item->overriden.value()
                    || box->overrides.get(item->name).has_value();
            }
        }
    }

    // 2. Go through all items and cache if the item values are mixed
    for (OverrideItemPtr& item : m_items) {
        update_overriden(item.get());
    }

    invoke_listeners<IListObserver<OverrideItem>>([&](IListObserver<OverrideItem>* l) {
        l->on_reset();
    });
}

void ObjectSettingsObservableList::set_value(const std::string& key, const Domain::ConfigValue& value)
{
    OverrideItem* item = find_item(key);

    ASSERT(item, "key has to correspond to existing item");

    if (item->is_override()) {
        for (Domain::ConfigBox* box : m_item_sources.at(item)) {
            box->overrides.set(key, value);
        }
        item->overriden = true;
    } else {
        for (Domain::ConfigBox* box : m_item_sources.at(item)) {
            box->items.opt(key).set(value);
        }
    }

    invoke_listeners<IListObserver<OverrideItem>>([&](IListObserver<OverrideItem>* l) {
        l->on_updated({m_item_index.at(key)});
    });
}

void ObjectSettingsObservableList::set_override(const std::string& key, bool enabled)
{
    OverrideItem* item = find_item(key);

    ASSERT(item && item->is_override(), "key has to correspond with existing override item");

    item->overriden = enabled;

    for (Domain::ConfigBox* box : m_item_sources.at(item)) {
        enabled ? box->overrides.enable(key) : box->overrides.disable(key);
    }

    invoke_listeners<IListObserver<OverrideItem>>([&](IListObserver<OverrideItem>* l) {
        l->on_updated({m_item_index.at(key)});
    });
}

OverrideItem* ObjectSettingsObservableList::find_item(const std::string& name)
{
    ItemMap::const_iterator it = m_item_index.find(name);

    return it == m_item_index.end() ? nullptr : m_items.at(it->second).get();
}

void ObjectSettingsObservableList::update_overriden(OverrideItem* item)
{
    const Domain::ConfigValue value = item->config_item->value();

    item->mixed = false;
    if (item->overriden.has_value()) { // override
        bool overriden = item->overriden.value();
        for (Domain::ConfigBox* source : m_item_sources.at(item)) {
            std::optional<Domain::ConfigItem> source_val = source->overrides.get(item->name);
            if (overriden != source_val.has_value()
                || value != source->overrides.find(item->name)->value())
            {
                item->mixed = true;
                break;
            }
        }
    } else { // item
        for (Domain::ConfigBox* source : m_item_sources.at(item)) {
            if (value != source->items.find(item->name)->value()) {
                item->mixed = true;
                break;
            }
        }
    }
}

} // namespace Slic3r::Biz
