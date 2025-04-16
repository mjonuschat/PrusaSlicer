#pragma once

#include <any>
#include <cfloat>
#include <concepts>
#include <exception>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Slic3r/Assert.hpp"

struct Percentage {
public:
    Percentage(double v) : value{ v } {}
    explicit operator double() const { return value; }
private:
    double value = 0.;
};

namespace Slic3r::Domain {

namespace detail {
    class ConfigItemValue;
}

template<typename T>
concept IsEnum = std::is_enum_v<T>;

enum class ConfigItemType
{
    None,
    Bool,
    Int,
    Double,
    String,
    Enum,
    FloatOrPercent,
    Percent,
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
    std::function<void(ConfigItem&)> init_fn;
    std::function<void(ConfigItem&, std::string_view box)> init_fn_ex;
    std::vector<std::string> belongs_to{ }; // Which box it belongs to. Can be None, in which case the latter cannot be.
    std::vector<std::string> belongs_to_optional{ }; // It is also here, but always nullable (overrides).
    
    // Enum specific:
    std::any enum_type; // holds an object of the required enum type.
    std::vector<EnumValueDef> enum_values; // Value (as int), serialized string and non-translated UI label.

    // Non-translated Label of the GUI input field. In case the GUI input fields are grouped in some views,
    // the label defines a short label of a grouped value, while full_label contains a label of a stand-alone field.
    // The full label is shown, when adding an override parameter for an object or a modified object.
    std::string label;
    std::string full_label;
    
    // Category of a configuration field, from the GUI perspective. One of: "Layers and Perimeters",
    // "Infill", "Support material", "Speed", "Extruders", "Advanced", "Extrusion Width"
    std::string category;
    std::string tooltip; // A tooltip text shown in the GUI.
    std::string sidetext; // Text right from the input field.
    std::string cli; // Format of this parameter on a command line.

    // Set for type == FloatOrPercent. It provides a link to a configuration value, of which this option
    // provides a ratio. E.g. external_perimeter_speed may be defined as a fraction of perimeter_speed.
    std::string ratio_over;
    
    // For text only:
    bool multiline  = false; // True for multiline strings.
    bool full_width = false; // For text input: If true, the GUI text box spans the complete page width.
    bool is_code    = false; // GUI formats text as code (fixed-width).
    int height          = -1; // Height of a multiline GUI text box.

    float min = -FLT_MAX; // <min, max> limit of a numeric input.
    float max =  FLT_MAX; // If not set, the <min, max> is set to <INT_MIN, INT_MAX>

    static constexpr const char* nocli = "~~nocli";

    // ARE THE FOLLOWING EVER USED?
    bool readonly = false; // Not editable. Currently only used for the display of the number of threads.
    int width = -1; // Optional width of an input field.
    std::vector<std::string> shortcut;

    // In case we want to show list of choices, the following holds pairs of value - GUI string.
    std::vector<std::pair<std::variant<int, double, std::string>, std::string>> choices;

    // NEEDS MORE WORK (TODO):
    int mode = 0; // Should really be an enum. It is not used right now, just need the defs to compile.
    std::vector<std::string> aliases; // We can probably clear them in all cases and start fresh. Legacy loading will handle them.
    double max_literal = 1; // // To check if it's not a typo and a % is missing - TODO Check how this is used.

    // NEEDS MORE WORK (TODO)
    // Usually empty. Otherwise "serialized" or "show_value"
    // The flags may be combined.
    // "serialized" - vector valued option is entered in a single edit field. Values are separated by a semicolon.
    // "show_value" - even if enum_values / enum_labels are set, still display the value, not the enum label.
    std::string gui_flags;

    enum class GUIType { // TODO Go through this one after everything is ported and remove what we don't use.
        undefined,
        i_enum_open,  // Open enums, integer value could be one of the enumerated values or something else.
        f_enum_open,  // Open enums, float value could be one of the enumerated values or something else.
        select_open,  // Open enums, string value could be one of the enumerated values or something else.
        color,        // Color picker, string value.
        one_string,   // Vector value, but edited as a single string.
        select_close, // Close parameter, string value could be one of the list values.
        password,     // Password, string vaule is hidden by asterisk.
    };
    GUIType gui_type = GUIType::undefined;
};


//max_literal









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
    bool operator==(const ConfigItem& other) const;
    bool operator!=(const ConfigItem& other) const { return ! (*this == other); }

    const ConfigItemDef& def() const { ASSERT(m_def); return *m_def; }

    bool is_vector() const { return m_type >= ConfigItemType::Bools; }
    void set_null(bool null);
    bool is_null() const;
    const std::string& name() const { return m_name; }
    ConfigItemType type() const { return m_type; }

    // Getters and setters for single config values.
    // Assert hard when the type does not match.
    template<class T> T get() const;
    template<class T> void set(T);

    // Getters and setters for specific cases.
    bool is_percent() const;

    // Enums getters and setters have same signature as the general ones, but they are 
    // defined here so that they can be instantiated for types not known in Config.cpp.
    template <IsEnum T>
    void set(T value)
    {
        ASSERT(m_type == ConfigItemType::Enum);
        ASSERT(typeid(T) == def().enum_type.type(), "Enum types mismatch.");
        set_enum_from_int(int(value));
    }
    template <IsEnum T>
    T get() const
    {
        ASSERT(m_type == ConfigItemType::Enum);
        ASSERT(typeid(T) == def().enum_type.type(), "Enum types mismatch.");
        return get_enum_as_int();
    }

    // Getter and setter for enums for use with serialized values.
    void set_enum_from_string(std::string_view value);
    std::pair<std::string_view, std::string_view> get_enum_strings() const;

    // Getters and setters for vector config options. They just
    // expose the underlying vector for now.
    template<class T> const std::vector<T>& vec() const { return const_cast<ConfigItem*>(this)->vec<T>(); }
    template<class T> std::vector<T>& vec();

    
private:
    std::string m_name{};
    ConfigItemType m_type{ ConfigItemType::None };
    bool m_is_nullable{ false };
    const ConfigItemDef* m_def{ nullptr };
    detail::ConfigItemValue* m_data{ nullptr };

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

    std::vector<std::string> diff_keys(const ConfigBox& other) const;


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
    template<class T>
    T get(const std::string_view key, int extruder_idx = -1) const {
        return opt(key, extruder_idx).get<T>();
    }

    bool is_null(const std::string_view key, int extruder_idx = -1) const {
        return opt(key, extruder_idx).is_null();
    }

    std::vector<std::string> diff_keys(const FullConfig& other) const;

    virtual std::string_view name() const = 0;
    virtual ~FullConfig() = default;

protected:
    FullConfig() = default;
    void add(const ConfigBox& box);
    void add(const std::vector<std::reference_wrapper<const ConfigBox>>& boxes);

private:
    std::map<std::string, ConfigItem> m_single_items;
    std::map<std::string, std::vector<ConfigItem>> m_multi_items;

    friend class ConfigView; // Ugly, but we can probably live with that.
    const ConfigItem& opt(const std::string_view key, int extruder_idx) const;
};





// To be used by backend to extract values for a given object while accounting
// for possible per-object / per volume overrides. Keeps references to objects
// used during its construction!
//
// IT DOES NOT SUPPORT E.G. MULTIPLE ObjectSettings PER EXTRUDER. DO WE NEED THAT?
class ConfigView
{
public:
    template <typename... Args>
    ConfigView(const FullConfig& fc, Args... args)
        : m_config_boxes{ args... }, m_full_config{fc} {}

    template<class T>
    T get(const std::string_view key, int extruder_idx = -1) const {
        return opt(key, extruder_idx).get<T>();
    }

    bool is_null(const std::string_view key, int extruder_idx = -1) const {
        return opt(key, extruder_idx).is_null();
    }

    std::vector<std::string> diff_keys(const ConfigView& other) const;


private:
    std::vector<std::reference_wrapper<const ConfigBox>> m_config_boxes;
    const FullConfig& m_full_config;

    const ConfigItem& opt(const std::string_view key, int extruder_idx) const;
};

} // namespace Slic3r::Domain
