#pragma once

#include <any>
#include <cfloat>
#include <concepts>
#include <exception>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/Types.hpp"


namespace Slic3r::Domain {

class Percentage {
public:
    bool operator==(const Percentage& other) const { return value == other.value; }
    bool operator<(const Percentage& other) const { return value < other.value; }
    double get_abs_value(double ratio_over) const { return (value / 100.) * ratio_over; }

    double value = 0.;
};



class FloatOrPercentage {
public:
    FloatOrPercentage() = default;
    FloatOrPercentage(const FloatOrPercentage&) = default;
    FloatOrPercentage(double value) : m_is_percentage{ false }, m_value { value } {}
    FloatOrPercentage(Percentage percentage) : m_is_percentage{ true }, m_value { percentage.value } {}

    bool is_percentage() const { return m_is_percentage; }
    bool is_zero() const { return m_value == 0.; }

    double float_value() const { ASSERT(! is_percentage()); return m_value; }
    Percentage percentage() const { ASSERT(is_percentage()); return Percentage{ m_value }; }

    double get_abs_value(double ratio_over) const { return (is_percentage() ? percentage().get_abs_value(ratio_over) : m_value); }

    bool operator==(const FloatOrPercentage& other) const { return m_value == other.m_value && m_is_percentage == other.m_is_percentage; }


private:
    double m_value = 0.;
    bool m_is_percentage = false;
};



enum class ConfigItemType
{
    None,           // Wraps around:
    Bool,           // bool
    Int,            // int
    IntOptional,    // std::optional<int>
    Double,         // double
    String,         // std::string
    Enum,           // int
    Point,          // Vec2d
    FloatOrPercent, // FloatOrPercentage
    Percent,        // Percentage
    // Vector types follow, Bools MUST be first (see ConfigItem::is_vector)
    Bools,          // std::vector<bool>
    Ints,           // std::vector<int>
    Doubles,        // std::vector<double>
    Strings,        // std::vector<std::string>
    Points,         // std::vector<Vec2d>
    Enums           // std::vector<int>
};



template<typename T>
concept IsEnum = std::is_enum_v<T>;

template<typename T>
concept StdVector = requires {
    typename T::value_type;
    typename T::allocator_type;
    requires std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;
};

template<typename T>
concept IsVectorOfEnums =
    StdVector<std::remove_cvref_t<T>> &&
    std::is_enum_v<typename std::remove_cvref_t<T>::value_type>;

template<typename T>
concept IsNotEnumOrVectorOfEnums = ! IsVectorOfEnums<T> && ! IsEnum<T>;

struct EnumValueDef
{
    int enum_value;
    std::string_view str_serialized;
    std::string_view str_ui;
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
    std::string location{ }; // Which box it belongs to. Must not be empty.
    std::vector<std::string> overrides_in{ }; // Which boxes this can be overridden in.
    
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

    ConfigItem(const ConfigItem& other);
    ConfigItem& operator=(const ConfigItem& other);
    bool operator==(const ConfigItem& other) const;
    bool operator!=(const ConfigItem& other) const { return ! (*this == other); }

    const ConfigItemDef& def() const { ASSERT(m_def); return *m_def; }

    bool is_vector() const { return m_type >= ConfigItemType::Bools; }
    void set_null(bool null);
    bool is_null() const;
    bool is_nullable() const { return m_is_nullable; }
    const std::string& name() const { return m_name; }
    ConfigItemType type() const { return m_type; }

    // Getters and setters. Assert hard when the type does not match.
    template<IsNotEnumOrVectorOfEnums T> const T& get() const;
    template<IsNotEnumOrVectorOfEnums T> void set(const T&);

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
    template <IsVectorOfEnums T>
    void set(T value)
    {
        ASSERT(m_type == ConfigItemType::Enums);
        ASSERT(typeid(T::value_type) == def().enum_type.type(), "Enum types mismatch.");
        std::vector<int> as_ints;
        for (size_t i=0; i<value.size(); ++i)
            as_ints.emplace_back(int(value[i]));
        set_enums_from_ints(as_ints);
    }
    template <IsVectorOfEnums T>
    T get() const
    {
        ASSERT(m_type == ConfigItemType::Enums);
        ASSERT(typeid(T::value_type) == def().enum_type.type(), "Enum types mismatch.");
        std::vector<int> values = get_enums_as_ints();
        T out;
        for (size_t i=0; i<values.size(); ++i)
            out.emplace_back(T::value_type(values[i]));
        return out;
    }
    // One helper to allow settings strings by string literals:
    void set(const char* str) { set(std::string(str)); }

    // Getter and setter for enums for use with serialized values.
    void set_enum_from_string(std::string_view value);
    std::pair<std::string_view, std::string_view> get_enum_strings() const;
    void set_enums_from_strings(std::vector<std::string_view> values);
    std::vector<std::pair<std::string_view, std::string_view>> get_enums_strings() const;

    // These methods expose the underlying vector to allow in-place modifications.
    template<class T> const std::vector<T>& vec() const { return const_cast<ConfigItem*>(this)->vec<T>(); }
    template<class T> std::vector<T>& vec();

    
private:
    std::string m_name{};
    ConfigItemType m_type{ ConfigItemType::None };
    bool m_is_nullable{ false }; // This is an override of something.
    bool m_is_null{ false };     // Whether it is currently overriding or not.
    const ConfigItemDef* m_def{ nullptr };
    std::any m_data;

    // Private setter/getter to avoid leaking implementation into header
    // through set_enum/get_enum templates.
    void set_enum_from_int(int value);
    int get_enum_as_int() const;
    void set_enums_from_ints(const std::vector<int>& values);
    std::vector<int> get_enums_as_ints() const;
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
    T get(const std::string_view key) const {
        return opt_single(key).get<T>();
    }

    template<StdVector T>
    T get(const std::string_view key) const {
        const auto single_item_it{m_single_items.find(std::string{key})};
        if (single_item_it != m_single_items.end()) {
            return single_item_it->second.get<T>();
        }

        T result;
        const std::vector<ConfigItem>& items{this->opt_multi(key)};
        std::transform(items.begin(), items.end(), std::back_inserter(result), [](const ConfigItem& item){
            return item.get<typename T::value_type>();
        });
        return result;
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

    const ConfigItem& opt_single(const std::string_view key) const;
    const std::vector<ConfigItem>& opt_multi(const std::string_view key) const;

    friend class ConfigView;
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
    T get(const std::string_view key) const {
        for (auto rev_it = m_config_boxes.rbegin(); rev_it != m_config_boxes.rend(); ++rev_it) {
            if (auto opt = rev_it->get().contains(key); opt.has_value() && ! (*opt)->is_null()) {
                return (**opt).get<T>();
            }
        }
        return m_full_config.get<T>(key);
    }

    std::vector<std::string> diff_keys(const ConfigView& other) const;


private:
    std::vector<std::reference_wrapper<const ConfigBox>> m_config_boxes;
    const FullConfig& m_full_config;
};

} // namespace Slic3r::Domain
