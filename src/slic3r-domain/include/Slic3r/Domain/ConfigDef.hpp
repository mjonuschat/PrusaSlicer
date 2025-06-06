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

using ConfigLocation = std::variant<FDMConfigLocation, SLAConfigLocation, PhysicalPrinterLocation>;

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

// A collection of definitions of all config items. ConfigItems will keep references into it,
// it has to outlive everything. For the same reason it is read-only after it is constructed.
// It is therefore safe to be used from multiple threads.
class ConfigDefinitions
{
public:
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
