#pragma once

#include <algorithm>
#include <boost/container_hash/hash.hpp>
#include <cfloat>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/ConfigValue.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"

namespace Slic3r::Domain {
// A wrapper type for a single config item. Not polymorphic, not templated. The caller
// shall not be bothered by dynamic casts, pointer ownership and other technicalities.
class ConfigItem
{
public:
    ConfigItem(const ConfigItemDef& def, std::string_view box_type);

    bool operator==(const ConfigItem& other) const = default;

    const ConfigItemDef& def() const { ASSERT(m_def); return *m_def; }

    void set_null(bool null);
    bool is_null() const;
    bool is_nullable() const;
    const std::string& name() const;

    template <typename T>
    T get() const
    {
        return m_data.get<T>();
    }

    template <typename T>
    void set(const T& value)
    {
        m_data.set(value);
    }

    std::size_t hash() const;

    template <typename Visitor>
    auto visit(Visitor&& visitor) const {
        return m_data.visit(std::forward<Visitor>(visitor));
    }

    template <typename T>
    bool holds_alternative() const {
        return m_data.holds_alternative<T>();
    }

private:
    std::string m_name{};
    bool m_is_nullable{ false }; // This is an override of something.
    bool m_is_null{ false };     // Whether it is currently overriding or not.
    const ConfigItemDef* m_def{ nullptr };
    ConfigValue m_data;
};


// A container storing some subset of config options. The constructor iterates through ConfigDefinititions and
// creates keys that are tagged as belonging in this ConfigBox type. No items can be removed or added later.
// This is a base class not designed to be used as it is (protected ctr).
class ConfigBox
{
public:
    const ConfigItem& opt(const std::string_view key) const { return const_cast<ConfigBox*>(this)->opt(key); }
    ConfigItem& opt(const std::string_view key);

    std::string_view type() const { return m_type; }
    std::optional<const ConfigItem*> contains(const std::string_view key) const;

    std::vector<ConfigItem>::iterator begin() { return m_items.begin(); }
    std::vector<ConfigItem>::iterator end() { return m_items.end(); }
    std::vector<ConfigItem>::const_iterator begin() const { return m_items.cbegin(); }
    std::vector<ConfigItem>::const_iterator end() const { return m_items.cend(); }

    std::vector<std::string> diff_keys(const ConfigBox& other) const;
    std::size_t hash() const;

    virtual ~ConfigBox() = default;

protected:
    ConfigBox(const ConfigDefinitions& defs, std::string_view type);

    std::vector<ConfigItem> m_items;
    std::string m_type{ };
};

using BoxRef = std::reference_wrapper<const ConfigBox>;
using BoxRefs = std::vector<BoxRef>;
using FullConfigInput = std::vector<std::variant<BoxRef, BoxRefs>>;


// Base class for a full config, which holds multiple config boxes and
// has const getters to get a ConfigItem by key.
// WARNING: This base class keeps pointers to ConfigBoxes passed to it in the constructor.
// It is the responsibility of the derived class to ensure that the ConfigBoxes stay alive.
class FullConfig {
public:
    template<typename T>
    T get(const std::string_view key) const {
        if constexpr (Domain::is_std_vector_v<T>) {
            const auto single_item_it{m_single_items.find(std::string{key})};
            if (single_item_it != m_single_items.end()) {
                return single_item_it->second.get<T>();
            }
            return get_multi<typename T::value_type>(key);
        } else {
            return opt_single(key).get<T>();
        }
    }

    std::size_t hash() const;

    virtual std::string_view name() const = 0;
    virtual ~FullConfig() = default;

protected:
    FullConfig(const FullConfigInput& boxes);

private:
    std::map<std::string, ConfigItem> m_single_items;
    std::map<std::string, std::vector<ConfigItem>> m_multi_items;
    std::vector<std::string> m_single_item_keys;
    std::vector<std::string> m_multi_item_keys;

    void add(const ConfigBox& box);
    void add(const BoxRefs& boxes);

    template <typename T>
    std::vector<T> get_multi(const std::string_view key) const {
        std::vector<T> result;
        const std::vector<ConfigItem>& items{this->opt_multi(key)};
        std::transform(items.begin(), items.end(), std::back_inserter(result), [](const ConfigItem& item){
            return item.get<T>();
        });
        return result;
    }

    const ConfigItem& opt_single(const std::string_view key) const;
    const std::vector<ConfigItem>& opt_multi(const std::string_view key) const;

    friend class ConfigView;
};

using ConfigBoxPtr = std::shared_ptr<const ConfigBox>;
using ConfigBoxesPtrs = std::vector<ConfigBoxPtr>;
using FullConfigPtr = std::shared_ptr<const FullConfig>;

// To be used by backend to extract values for a given object while accounting
// for possible per-object / per volume overrides. Keeps references to objects
// used during its construction!
//
// IT DOES NOT SUPPORT E.G. MULTIPLE ObjectSettings PER EXTRUDER. DO WE NEED THAT?
class ConfigView
{
public:
    ConfigView(const FullConfigPtr& full_config, const ConfigBoxesPtrs& config_boxes)
        : m_config_boxes{config_boxes}, m_full_config{full_config}
    {
        ASSERT(m_full_config);
        for (const ConfigBoxPtr& ptr : m_config_boxes) {
            ASSERT(ptr);
        }
    }

    template<class T>
    T get(const std::string_view key) const {
        // TODO, this is horrible!
        if constexpr (!Domain::is_std_vector_v<T>) {
            for (auto rev_it = m_config_boxes.rbegin(); rev_it != m_config_boxes.rend(); ++rev_it) {
                const ConfigBoxPtr& override{*rev_it};
                if (auto opt = override->contains(key); opt.has_value() && ! (*opt)->is_null()) {
                    return (**opt).get<T>();
                }
            }
        }
        return m_full_config->get<T>(key);
    }

    std::vector<std::string> diff_keys(const ConfigView& other) const;

    bool operator==(const ConfigView& other) const;

    std::size_t hash() const;

protected:
    ConfigBoxesPtrs m_config_boxes;
    FullConfigPtr m_full_config;

private:

    const ConfigItem& opt_single(std::string_view key) const {
        for (auto rev_it = m_config_boxes.rbegin(); rev_it != m_config_boxes.rend(); ++rev_it) {
            const ConfigBoxPtr& override{*rev_it};
            if (auto opt = override->contains(key); opt.has_value() && ! (*opt)->is_null()) {
                return (**opt);
            }
        }
        return m_full_config->opt_single(key);
    }

    const std::vector<ConfigItem>& opt_multi(std::string_view key) const {
        // Multi keys cannot be overriden.
        return m_full_config->opt_multi(key);
    }
};

} // namespace Slic3r::Domain
