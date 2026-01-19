#pragma once

#include <boost/container_hash/hash.hpp>
#include <cfloat>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "Slic3r/Domain/ConfigValue.hpp"
#include "Slic3r/Domain/PrinterTechnology.hpp"

namespace Slic3r::Domain {

class ConfigDefinitions;

enum class FDMConfigLocation {
    None,
    Printer,
    Tool,
    Print,
    Filament,
    Project,
    Object,
    Volume,
};

enum class SLAConfigLocation {
    None,
    Printer,
    Material,
    Print,
    Object,
};

struct PhysicalPrinterLocation {
    bool operator==(const PhysicalPrinterLocation&) const = default;
    bool operator<(const PhysicalPrinterLocation&) const {
        return false;
    }
};

struct AppConfigLocation {
    bool operator==(const AppConfigLocation&) const = default;
    bool operator<(const AppConfigLocation&) const {
        return false;
    }
};

using ConfigLocation = std::variant<FDMConfigLocation, SLAConfigLocation, PhysicalPrinterLocation, AppConfigLocation>;

std::string get_location_name(const ConfigLocation& location);

// Static definition of a single config item. Contains all info about what the item is,
// as well as rules for diplaying it in the UI. All information about the item is here,
// including where it is supposed to be.
struct ConfigItemDef
{
    bool operator<(const ConfigItemDef& other) const { return name < other.name; }

    std::string name{};
    const std::type_info* type{ nullptr };
    std::function<ConfigValue()> init_fn;
    std::function<ConfigValue(const ConfigLocation& config_location)> init_fn_ex;
    ConfigLocation location; // Which box it belongs to. Must not be empty.
    std::set<ConfigLocation> overrides_in; // Which boxes this can be overridden in.

    // Non-translated Label of the GUI input field. In case the GUI input fields are grouped in some views,
    // the label defines a short label of a grouped value, while full_label contains a label of a stand-alone field.
    // The full label is shown, when adding an override parameter for an object or a modified object.
    std::string label;
    std::string full_label;

    enum class Category : uint8_t
    {
        Unkown              = 0, ///< Default category, throws an error
        Hidden              = 1, ///< Hidden from user, not visible in GUI
        General             = 2,
        Material            = 3,
        Cooling             = 4,
        Bed                 = 5,
        CustomGcode         = 6,
        MachineLimits       = 7,
        LayersAndPerimeters = 8,
        Infill              = 9,
        SkirtAndBrim        = 10,
        SupportMaterial     = 11,
        Speed               = 12,
        Extruders           = 13,
        MultipleExtruders   = 14,
        Advanced            = 15, ///< Last explicitly ordered category.
        ///< Note: Ordering is very important for correct category ordering in the Settings window.
        ///< Items below do not have a defined position, therefore they do not have explicit values.
        WipeOptions,
        OutputOptions,
        SingleExtruderMMSetup,
        Pad,
        Supports,
        Hollowing,
        Notes,
        MaterialPrintingProfile,
        PreferencesGeneral,
        Appearance,
        Camera,
        Services,
    };

    static std::string translate_category(Category category, const PrinterTechnology pt);

    // Category of a configuration field, from the GUI perspective. One of: "Layers and Perimeters",
    // "Infill", "Support material", "Speed", "Extruders", "Advanced", "Extrusion Width"
    Category category = Category::Unkown;
    std::string option_group;
    std::string row_group;
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

    std::optional<double> min; // <min, max> limit of a numeric input.
    std::optional<double> max; // If not set, the <min, max> is set to <INT_MIN, INT_MAX>

    bool require_tool_parity = false; // Requires number of elements to be same as number of tools

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
        i_enum_open,  ///< Open enums, integer value could be one of the enumerated values or something else.
        f_enum_open,  ///< Open enums, float value could be one of the enumerated values or something else.
        s_enum_open,  ///< Open enums, string value could be one of the enumerated values or something else.
        color,        ///< Color picker, string value.
        one_string,   ///< @deprecated Vector value, but edited as a single string.
        select_close, ///< @deprecated Close parameter, string value could be one of the list values.
        password,     ///< Password, string vaule is hidden by asterisk.
        textfield,
        textfields,
        checkbox,
        checkboxes,
        spinbox,
        spinboxes,
        combobox,
        comboboxes,
        points,
        file_picker,
        bed_shape,
        substitutions,
        ramming_params,
        extruder_selection
    };
    GUIType gui_type = GUIType::undefined;
};

// A collection of definitions of all config items. ConfigItems will keep references into it,
// it has to outlive everything. For the same reason it is read-only after it is constructed.
// It is therefore safe to be used from multiple threads.
class ConfigDefinitions
{
public:
    ConfigDefinitions() = delete;
    ConfigDefinitions(const ConfigDefinitions&) = delete;
    ConfigDefinitions(ConfigDefinitions&&) = delete;
    ConfigDefinitions& operator=(const ConfigDefinitions&) = delete;
    ConfigDefinitions& operator=(ConfigDefinitions&&) = delete;

    ConfigDefinitions(const std::set<ConfigLocation>& acceptable_boxes, std::function<void(ConfigDefinitions&)> init_fn);
    const std::vector<ConfigItemDef>& defs() const { return m_defs; }

    // Add a config definition. Calling this after ctr finishes is an error.
    ConfigItemDef* add(const std::string_view name, const std::type_info& type);

private:
    void check_valid() const;
    std::vector<ConfigItemDef> m_defs;
    bool m_finalized{ false };
    std::set<ConfigLocation> m_acceptable_boxes;
};
}
