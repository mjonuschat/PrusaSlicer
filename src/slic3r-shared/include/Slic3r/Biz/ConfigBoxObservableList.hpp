///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Domain/Config.hpp>
#include <Slic3r/Biz/IObservableList.hpp>

namespace Slic3r::Biz {

class ConfigBoxObservableList : public IObservableList<Domain::ConfigItem>
{
public:
    void set_config_box(Domain::ConfigBox* config_box)
    {
        if (m_config_box != config_box) {
            m_config_box = config_box;
            invoke_listeners<IListObserver<Domain::ConfigItem>>(
                [&](IListObserver<Domain::ConfigItem>* l) { l->on_reset(); }
            );
        }
    }

    const Domain::ConfigItem& at(size_t index) const override
    {
        return m_config_box->items.all_items().at(index);
    }

    size_t size() const override
    {
        return m_config_box->items.all_items().size();
    }

    void set_value(const std::string_view key, const Domain::ConfigValue& value)
    {
        const std::vector<Domain::ConfigItem>& all_items = m_config_box->items.all_items();

        std::vector<Domain::ConfigItem>::const_iterator index = std::find_if(
            all_items.cbegin(),
            all_items.cend(),
            [key](const Domain::ConfigItem& item) { return item.def().name == key; }
        );

        ASSERT(index != all_items.cend());

        m_config_box->items.opt(key).set(value);

        invoke_listeners<IListObserver<Domain::ConfigItem>>([&](IListObserver<Domain::ConfigItem>* l) {
            l->on_updated(std::distance(all_items.cbegin(), index));
        });
    }

private:
    Domain::ConfigBox* m_config_box{nullptr};
};

class ConfigBoxOverridesObservableList : public IObservableList<Domain::ConfigItem>
{
public:
    void set_config_box(Domain::ConfigBox* config_box)
    {
        if (m_config_box != config_box) {
            m_config_box = config_box;
            invoke_listeners<IListObserver<Domain::ConfigItem>>(
                [&](IListObserver<Domain::ConfigItem>* l) { l->on_reset(); }
            );
        }
    }

    const Domain::ConfigItem& at(size_t index) const override {
        return m_config_box->overrides.all_items().at(index);
    }

    size_t size() const override {
        return m_config_box->overrides.all_items().size();
    }

    void set_value(const std::string& key, const Domain::ConfigValue& value)
    {
        const std::vector<Domain::ConfigItem>& all_items = m_config_box->overrides.all_items();

        std::vector<Domain::ConfigItem>::const_iterator index = std::find_if(
            all_items.cbegin(),
            all_items.cend(),
            [key](const Domain::ConfigItem& item) { return item.def().name == key; }
        );

        ASSERT(index != all_items.cend());

        m_config_box->overrides.set(key, value);

        invoke_listeners<IListObserver<Domain::ConfigItem>>([&](IListObserver<Domain::ConfigItem>* l) {
            l->on_updated(std::distance(all_items.cbegin(), index));
        });
    }

private:
    Domain::ConfigBox* m_config_box{nullptr};
};

} // namespace Slic3r::Biz
