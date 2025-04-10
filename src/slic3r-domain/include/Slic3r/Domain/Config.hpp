#pragma once

#include <any>
#include <exception>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "Slic3r/Assert.hpp"


class ConfigItemValue;


enum class ConfigItemType
{
    None,
    Bool,
    Int,
    Double,
    String,
    Enum,
    FloatOrPercent,
    Bools, // Vector types follow, Bools MUST be first (see ConfigItem::is_vector)
    Ints,
    Doubles,
    Strings
};


struct EnumValueDef
{
    int enum_value;
    std::string str_serialized;
    std::string str_ui;
    bool operator<(const EnumValueDef& other) const { return enum_value < other.enum_value; }
};


class ConfigItem;

// Static definition of a single config item. Contains all info about what the item is,
// as well as rules for diplaying it in the UI. All information about the item is here,
// including where it is supposed to be.
struct ConfigItemDef
{
	bool operator<(const ConfigItemDef& other) const { return name < other.name; }

    std::string name{};
    ConfigItemType type{ ConfigItemType::None };

    // Sets default.
    std::function<void(ConfigItem&)> init_fn;

    // Sets default, possibly different per box.
    std::function<void(ConfigItem&, std::string_view box)> init_fn_ex;

    // Which box it belongs to. Can be None, in which case the latter cannot be.
    std::vector<std::string> belongs_to{ };

    // It is also here, but always nullable (overrides).
    std::vector<std::string> belongs_to_optional{ };

    // For enums. std::any holding an object of the required enum type.
    std::any enum_type;

    // For enums. Value (as int), serialized string and UI label.
    std::vector<EnumValueDef> enum_values;

};




// A collection of definitions of all config items. ConfigItems will keep references into it,
// it has to outlive everything. For the same reason it is read-only after it is constructed.
// It is therefore safe to be used from multiple threads.
class ConfigDefinitions
{
public:
    ConfigDefinitions(const std::vector<std::string>& acceptable_boxes, std::function<void(ConfigDefinitions&)> init_fn);
    const std::vector<ConfigItemDef>& defs() const { return m_defs; }

	// Add a config definition. Calling this after ctr finishes is an error.
    ConfigItemDef* add(const std::string_view name, ConfigItemType type);

private:
    void check_valid() const;
    std::vector<ConfigItemDef> m_defs;
    std::vector<std::string> m_acceptable_boxes;
	bool m_finalized{ false };
};



// A wrapper type for a single config item. Not polymorphic, not templated. The caller
// shall not be bothered by dynamic casts, pointer ownership and other technicalities.
class ConfigItem
{
public:
    ConfigItem(const ConfigItemDef& def, std::string_view box_type);

    ~ConfigItem();
    ConfigItem(const ConfigItem& other);
    ConfigItem& operator=(const ConfigItem& other);
    bool operator<(const ConfigItem& other) const { return name() < other.name(); }

    const ConfigItemDef& def() const { ASSERT(m_def); return *m_def; }

    bool is_vector() const { return m_type >= ConfigItemType::Bools; }
    void set_null(bool null);
    bool is_null() const;
    const std::string& name() const { return m_name; }
    ConfigItemType type() const { return m_type; }

    // Getters and setters. Assert hard when the type does not match.
    void set_bool(bool value);
    bool get_bool() const;

    void set_int(int value);
    int  get_int() const;

    void set_double(double value);
    double get_double() const;
    
	void set_percent(double value);
    double get_percent() const;
	bool is_percent() const;

    void set_str(const std::string& value);
    const std::string& get_str() const;

    template <typename T>
    void set_enum(T value)
    {
        static_assert(std::is_enum_v<T>);
        ASSERT(m_type == ConfigItemType::Enum);
        ASSERT(typeid(T) == def().enum_type.type(), "Enum types mismatch.");
        set_enum_from_int(int(value));
    }
    template <typename T>
    T get_enum() const
    {
        static_assert(std::is_enum_v<T>);
        ASSERT(m_type == ConfigItemType::Enum);
        ASSERT(typeid(T) == def().enum_type.type(), "Enum types mismatch.");
        return get_enum_as_int();
    }

    void set_enum_from_string(std::string_view value);
    std::pair<std::string_view, std::string_view> get_enum_strings() const;


    // Vector setters and getters. Exposes the underlying vector (if the type matches).
    std::vector<bool>& bools();
    const std::vector<bool>& bools() const;

    std::vector<int>& ints();
    const std::vector<int>& ints() const;

    std::vector<double>& doubles();
    const std::vector<double>& doubles() const;

    std::vector<std::string>& strings();
    const std::vector<std::string>& strings() const;
    
private:
    std::string m_name{};
    ConfigItemType m_type{ ConfigItemType::None };
    bool m_is_nullable{ false };
    const ConfigItemDef* m_def{ nullptr };
    ConfigItemValue* m_data{ nullptr };

    // Private setter/getter to avoid leaking implementation into header
    // through set_enum/get_enum templates.
    void set_enum_from_int(int value);
    int get_enum_as_int() const;
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
    std::optional<const ConfigItem*> has(const std::string_view key) const;

    std::vector<ConfigItem>::iterator begin() { return m_items.begin(); }
    std::vector<ConfigItem>::iterator end() { return m_items.end(); }
    std::vector<ConfigItem>::const_iterator begin() const { return m_items.cbegin(); }
    std::vector<ConfigItem>::const_iterator end() const { return m_items.cend(); }


protected:
    ConfigBox(const ConfigDefinitions& defs, std::string_view type);

    std::vector<ConfigItem> m_items;
    std::string m_type{ };
};



// Base class for a full config, which holds multiple config boxes and
// has const getters to get a ConfigItem by key.
// WARNING: This base class keeps pointers to ConfigBoxes passed to it in the constructor.
// It is the responsibility of the derived class to ensure that the ConfigBoxes stay alive.
class FullConfig {
public:
	const ConfigItem& opt(const std::string_view key) const {
		auto it = m_single_items.find(std::string(key));
        ASSERT(it != m_single_items.end());
		return *it->second;
	}
    const ConfigItem& opt(const std::string_view key, size_t extruder_idx) const {
        auto it = m_multi_items.find(std::string(key));
        ASSERT(it != m_multi_items.end());
        ASSERT(extruder_idx < it->second.size());
		return *(it->second[extruder_idx]);
    }
    virtual ~FullConfig() = default;

protected:
	FullConfig() = default;
    void add(const ConfigBox* box);
    void add(const std::vector<const ConfigBox*> boxes);

private:
    std::map<std::string, const ConfigItem*> m_single_items;
    std::map<std::string, std::vector<const ConfigItem*>> m_multi_items;
};





// To be used by backend to extract values for a given object while accounting
// for possible per-object / per volume overrides. Keeps references to objects
// used during its construction!
class ConfigView
{
public:
    template <typename... Args>
    ConfigView(const FullConfig& fc, Args... args)
        : m_config_boxes{ args... }, m_full_config{fc} {}
    
    const ConfigItem& opt(const std::string_view key) const;

private:
	std::vector<ConfigBox*> m_config_boxes;
    const FullConfig* m_full_config;
};
