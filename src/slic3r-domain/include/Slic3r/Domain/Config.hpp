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

#include "Slic3r/Domain/ConfigValue.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"

namespace Slic3r::Domain {
// A wrapper type for a single config item. Not polymorphic, not templated. The caller
// shall not be bothered by dynamic casts, pointer ownership and other technicalities.
class ConfigItem
{
public:
    ConfigItem(const ConfigItemDef& def, const ConfigLocation locaiton);

    bool operator==(const ConfigItem&) const = default;

    const ConfigItemDef& def() const {
        return *m_def;
    }

    template <typename T>
    T get() const
    {
        return m_value.get<T>();
    }

    template <typename T>
    void set(const T& value)
    {
        m_value.set(value);
    }

    template <typename Visitor>
    auto visit(Visitor&& visitor) const {
        return m_value.visit(std::forward<Visitor>(visitor));
    }

    ConfigValue value() const {
        return m_value;
    }

    template <typename T>
    bool holds_alternative() const {
        return m_value.holds_alternative<T>();
    }

    const std::string& name() const {
        return m_def->name;
    }

private:
    ConfigValue m_value;

    // Comparision operator, compares the defintion pointers.
    // It cannot be nullptr.
    const ConfigItemDef* m_def;
};

class ConfigItems
{
public:
    ConfigItems(const ConfigDefinitions& defs, const ConfigLocation& location);

    const ConfigItem& opt(const std::string_view key) const;
    ConfigItem& opt(const std::string_view key);

    ConfigItem* contains(const std::string& key);
    const ConfigItem* contains(const std::string& key) const;

    std::vector<ConfigItem>::iterator begin();
    std::vector<ConfigItem>::iterator end();
    std::vector<ConfigItem>::const_iterator begin() const;
    std::vector<ConfigItem>::const_iterator end() const;

    virtual ~ConfigItems() = default;

private:
    std::vector<ConfigItem> m_items;
};

class ConfigOverrides {
public:
    ConfigOverrides(const ConfigDefinitions& defs, const ConfigLocation location);

    template <typename T>
    void set(const std::string& key, const T& value) {
        const auto item_index{find(key)};
        m_items.at(item_index).set(value);
        m_used_overrides.insert({key, item_index});
    }

    void disable(const std::string& key);

    void enable(const std::string& key);

    std::size_t size() const;

    const bool empty() const;

    std::optional<ConfigItem> get(const std::string& key) const;

    ConfigItem* contains(const std::string& key);
    const ConfigItem* contains(const std::string& key) const;

    std::vector<std::reference_wrapper<const ConfigItem>> overriden_items() const;

private:
    std::size_t find(const std::string& key);

    std::map<std::string, std::size_t> m_used_overrides;
    std::vector<ConfigItem> m_items;
};

struct ContainsResult {
    ConfigItem* item{nullptr};
    bool is_override{};
};

struct ConstContainsResult {
    const ConfigItem* item{nullptr};
    bool is_override{};
};

struct ConfigBox
{
    ConfigItems items;
    ConfigOverrides overrides;
    ConfigLocation location;

    ContainsResult contains(const std::string& key) {
        if (auto* item{overrides.contains(key)}) {
            return {item, true};
        }
        return {items.contains(key), false};
    }

    ConstContainsResult contains(const std::string& key) const {
        if (auto* item{overrides.contains(key)}) {
            return {item, true};
        }
        return {items.contains(key), false};
    }

protected:
    ConfigBox(const ConfigDefinitions& defs, const ConfigLocation& location)
        : items{defs, location}, overrides{defs, location}, location{location}
    {}
};

using BoxRef = std::reference_wrapper<const ConfigBox>;
using BoxRefs = std::vector<BoxRef>;
using BoxOrBoxesVector = std::vector<std::variant<BoxRef, BoxRefs>>;

using LocationSize = std::optional<std::size_t>;
using ConfigLocationSizes = std::map<ConfigLocation, LocationSize>;

class SquashedConfig {
public:
    SquashedConfig(const BoxOrBoxesVector& boxes, const ConfigLocationSizes& sizes);

    std::vector<std::string> diff_keys(const SquashedConfig& other) const;

    bool operator==(const SquashedConfig& other) const;

    const std::map<std::string, ConfigValue>& values() const;

protected:
    std::size_t hash() const;
    std::map<std::string, ConfigValue> m_values;

private:
    void add(const ConfigBox& box, const ConfigLocationSizes& location_sizes);
    void add(const BoxRefs& boxes, const ConfigLocationSizes& location_sizes);
};

class FullConfig : public SquashedConfig {
public:
    template<typename T>
    T get(const std::string& key) const {
        return get_value(key).get<T>();
    }

    std::vector<std::string> keys() const;

    virtual ~FullConfig() = default;
protected:
    FullConfig(const BoxOrBoxesVector& input, const ConfigLocationSizes& location_sizes);

private:
    friend class ConfigView;
    ConfigValue get_value(const std::string& key) const;

    std::vector<std::string> m_keys;
};

class PartialConfig : public SquashedConfig{
public:
    template<typename T>
    std::optional<T> get(const std::string& key) const {
        if (const auto value{get_value(key)}) {
            return value->get<T>();
        }
        return std::nullopt;
    }

    // TODO: Remove this once possible!!! it allows changing the slicing input!
    template<typename T>
    void set(const std::string& key, const T& value) {
        // extremelly dangerous, anything can be set...
        m_values.insert_or_assign(key, ConfigValue{value});
    }

    virtual ~PartialConfig() = default;


protected:
    PartialConfig(const BoxOrBoxesVector& input, const ConfigLocationSizes& location_sizes);

private:
    friend class ConfigView;
    std::optional<ConfigValue> get_value(const std::string& key) const;
};


using FullConfigPtr = std::shared_ptr<const FullConfig>;
using PartialConfigPtr = std::shared_ptr<const PartialConfig>;

class ConfigView
{
public:
    ConfigView(FullConfigPtr full_config, const std::vector<PartialConfigPtr>& partial_configs);

    template<class T>
    T get(const std::string& key) const {
        return get_value(key).get<T>();
    }

    bool operator==(const ConfigView& other) const;

    std::size_t hash() const;

protected:
    FullConfigPtr m_full_config;
    std::vector<PartialConfigPtr> m_partial_configs;

private:
    ConfigValue get_value(const std::string& key) const;
};

} // namespace Slic3r::Domain
