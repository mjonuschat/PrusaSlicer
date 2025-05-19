#include "Slic3r/Domain/ConfigFDM.hpp"

#include "Slic3r/Domain/Types.hpp"

#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"

namespace Slic3r::Domain {

namespace {
template<typename T>
inline BoxRefs convert_to_box_refs(
    const std::vector<std::reference_wrapper<T>>& settings
)
{
    BoxRefs result;
    result.insert(result.end(), settings.begin(), settings.end());
    return result;
}

inline FullConfigInput convert_to_full_config_input(
    const PrinterSettings& printer_s,
    const std::vector<std::reference_wrapper<const ToolPrintSettings>>& tool_print_s,
    const PrintSettings& print_s,
    const std::vector<std::reference_wrapper<const FilamentSettings>>& filament_s,
    const ProjectSettings& project_s
) {
    ASSERT(filament_s.size() == tool_print_s.size());
    FullConfigInput result;
    result.push_back(printer_s);
    result.push_back(convert_to_box_refs(tool_print_s));
    result.push_back(print_s);
    result.push_back(convert_to_box_refs(filament_s));
    result.push_back(project_s);
    return result;
}
}

FullConfigFDM::FullConfigFDM(
    const PrinterSettings& printer_s,
    const std::vector<std::reference_wrapper<const ToolPrintSettings>>& tool_print_s,
    const PrintSettings& print_s,
    const std::vector<std::reference_wrapper<const FilamentSettings>>& filament_s,
    const ProjectSettings& project_s
)
    : FullConfig{convert_to_full_config_input(printer_s, tool_print_s, print_s, filament_s,  project_s)}
{}

// Implementation of FDM configs is done in this file.

// Define our own marking functions, the regular ones are not accessible in Domain.
static const std::string& L(const std::string& s) { return s; }
static const std::string& L_CONTEXT(const std::string& s, const std::string& ctx) { return s; }

void fdm_config_init_fn(ConfigDefinitions& defs);

// Define the static object holding all definitions. Provide list of acceptable
// boxes and the init function.
ConfigDefinitions s_defs_fdm({"printer_settings", "filament_settings", "print_settings",
    "toolprint_settings", "object_settings", "volume_settings", "project_settings"}, fdm_config_init_fn);


// JUST TEMPORARY UNTIL WE DECIDE WHAT TO DO WITH MODES.
// Right now, let's just define the constants so the defs compile.
enum { comSimple, comAdvanced, comExpert };



// Little helper to save some typing:
#define SET_DEFAULT(v) def->init_fn = [](ConfigItem& item) { item.set(v); };

// Now define the init function. This function will be called by ConfigDefinitions
// constructor and will fill the definitions with all the necessary data.
void fdm_config_init_fn(ConfigDefinitions& defs)
{
    using ConfigItemType::Bool;
    using ConfigItemType::Int;
    using ConfigItemType::IntOptional;
    using ConfigItemType::Double;
    using ConfigItemType::String;
    using ConfigItemType::Enum;
    using ConfigItemType::Point;
    using ConfigItemType::Percent;
    using ConfigItemType::FloatOrPercent;
    using ConfigItemType::Bools;
    using ConfigItemType::Ints;
    using ConfigItemType::Doubles;
    using ConfigItemType::Strings;
    using ConfigItemType::Points;

    ConfigItemDef* def = nullptr;

    init_common_fdm_sla_config_items(defs, "FDM");

    /* TODO - where does this belong to ?
    def = defs.add("profile_vendor", String);
    def->label = L("Profile vendor");
    def->tooltip = L("Name of profile vendor");
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT("");

    def = defs.add("profile_version", String);
    def->label = L("Profile version");
    def->tooltip = L("Version of profile");
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT("");

    // temporary workaround for compatibility with older Slicer
    {
        def = defs.add("preset_name", String);
        SET_DEFAULT("");
    }*/




// Defs from void PrintConfigDef::init_fff_params() follow:
    def = defs.add("arc_fitting", Enum);
    def->location = "print_settings";
    def->label = L("Arc fitting");
    def->tooltip = L("Enable to get a G-code file which has G2 and G3 moves. "
                     "G-code resolution will be used as the fitting tolerance.");
    def->enum_type = ArcFittingType::Disabled;
    def->enum_values = { { int(ArcFittingType::Disabled), "disabled", L("Disabled") },
                         { int(ArcFittingType::EmitCenter), "emit_center", L("Enabled: G2/3 I J") } };
    def->mode = comAdvanced;
    SET_DEFAULT(ArcFittingType::Disabled);

    def = defs.add("automatic_extrusion_widths", Bool);
    def->location = "print_settings";
    def->label = L("Automatic extrusion widths calculation");
    def->category = L("Extrusion Width");
    def->tooltip = L("Automatically calculates extrusion widths based on the nozzle diameter of the currently used extruder. "
                     "This setting is essential for printing with different nozzle diameters.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("automatic_infill_combination", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Automatic infill combination");
    def->category = L("Infill");
    def->tooltip = L("This feature automatically combines infill of several layers and speeds up your print by extruding thicker "
                     "infill layers while preserving thin perimeters, thus maintaining accuracy.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("automatic_infill_combination_max_layer_height", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Automatic infill combination - Max layer height");
    def->category = L("Infill");
    def->tooltip = L("Maximum layer height for combining infill when automatic infill combining is enabled. "
                     "Maximum layer height could be specified either as an absolute in millimeters value or as a percentage of nozzle diameter. "
                     "For printing with different nozzle diameters, it is recommended to use percentage value over absolute value.");
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage(Percentage{100.}));

    // Maximum extruder temperature, bumped to 1500 to support printing of glass.
    const int max_temp = 1500;

    def = defs.add("avoid_crossing_curled_overhangs", Bool);
    def->location = "print_settings";
    def->label = L("Avoid crossing curled overhangs (Experimental)");
    // TRN PrintSettings: "Avoid crossing curled overhangs (Experimental)"
    def->tooltip = L("Plan travel moves such that the extruder avoids areas where the filament may be curled up. "
                   "This is mostly happening on steeper rounded overhangs and may cause a crash with the nozzle. "
                   "This feature slows down both the print and the G-code generation.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("avoid_crossing_perimeters", Bool);
    def->location = "print_settings";
    def->label = L("Avoid crossing perimeters");
    def->tooltip = L("Optimize travel moves in order to minimize the crossing of perimeters. "
                   "This is mostly useful with Bowden extruders which suffer from oozing. "
                   "This feature slows down both the print and the G-code generation.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("avoid_crossing_perimeters_max_detour", FloatOrPercent);
    def->location = "print_settings";
    def->label = L("Avoid crossing perimeters - Max detour length");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("The maximum detour length for avoid crossing perimeters. "
                     "If the detour is longer than this value, avoid crossing perimeters is not applied for this travel path. "
                     "Detour length could be specified either as an absolute value or as percentage (for example 50%) of a direct travel path.");
    def->sidetext = L("mm or % (zero to disable)");
    def->min = 0;
    def->max_literal = 1000;
    def->mode = comExpert;
    SET_DEFAULT(FloatOrPercentage{0.});

    def = defs.add("bed_temperature", Int);
    def->location = "filament_settings";
    def->label = L("Other layers");
    def->tooltip = L("Bed temperature for layers after the first one. "
                   "Set this to zero to disable bed temperature control commands in the output.");
    def->sidetext = L("°C");
    def->full_label = L("Bed temperature");
    def->min = 0;
    def->max = 300;
    SET_DEFAULT(0);

    def = defs.add("chamber_temperature", Int);
    def->location = "filament_settings";
    // TRN: Label of a configuration parameter: Nominal chamber temperature.
    def->label = L("Nominal");
    def->full_label = L("Chamber temperature");
    def->tooltip = L("Required chamber temperature for the print.\nWhen set to zero, "
                     "the nominal chamber temperature is not set in the G-code.");
    def->sidetext = L("°C");
    def->min = 0;
    def->max = 1000;
    def->mode = comExpert;
    SET_DEFAULT(0);

    def = defs.add("chamber_minimal_temperature", Int);
    def->location = "filament_settings";
    // TRN: Label of a configuration parameter: Minimal chamber temperature
    def->label = L("Minimal");
    def->full_label = L("Chamber minimal temperature");
    def->tooltip = L("Minimal chamber temperature that the printer waits for before the print starts. This allows "
                     "to start the print before the nominal chamber temperature is reached.\nWhen set to zero, "
                     "the minimal chamber temperature is not set in the G-code.");
    def->sidetext = L("°C");
    def->min = 0;
    def->max = 1000;
    def->mode = comExpert;
    SET_DEFAULT(0);

    def = defs.add("bed_temperature_extruder", Int);
    def->location = "print_settings";
    def->label = L("Bed temperature by extruder");
    def->category = L("Extruders");
    def->tooltip = L("The extruder which determines bed temperatures. "
                     "Set to 0 to determine temperatures based on the first printing extruder "
                     "of the first and the second layers.");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0);

    def = defs.add("before_layer_gcode", String);
    def->location = "printer_settings";
    def->label = L("Before layer change G-code");
    def->tooltip = L("This custom code is inserted at every layer change, right before the Z move. "
                   "Note that you can use placeholder variables for all Slic3r settings as well "
                   "as [layer_num] and [layer_z].");
    def->multiline = true;
    def->full_width = true;
    def->height = 5;
    def->mode = comExpert;
    SET_DEFAULT("");

    def = defs.add("between_objects_gcode", String);
    def->location = "printer_settings";
    def->label = L("Between objects G-code");
    def->tooltip = L("This code is inserted between objects when using sequential printing. By default extruder and bed temperature are reset using non-wait command; however if M104, M109, M140 or M190 are detected in this custom code, Slic3r will not add temperature commands. Note that you can use placeholder variables for all Slic3r settings, so you can put a \"M109 S[first_layer_temperature]\" command wherever you want.");
    def->multiline = true;
    def->full_width = true;
    def->height = 12;
    def->mode = comExpert;
    SET_DEFAULT("");

    def = defs.add("bottom_solid_layers", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    //TRN Print Settings: "Bottom solid layers"
    def->label = L_CONTEXT("Bottom", "Layers");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Number of solid layers to generate on bottom surfaces.");
    def->full_label = L("Bottom solid layers");
    def->min = 0;
    SET_DEFAULT(3);

    def = defs.add("bottom_solid_min_thickness", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L_CONTEXT("Bottom", "Layers");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("The number of bottom solid layers is increased above bottom_solid_layers if necessary to satisfy "
    				 "minimum thickness of bottom shell.");
    def->full_label = L("Minimum bottom shell thickness");
    def->sidetext = L("mm");
    def->min = 0;
    SET_DEFAULT(0.);

    def = defs.add("bridge_acceleration", Double);
    def->location = "print_settings";
    def->label = L("Bridge");
    def->tooltip = L("This is the acceleration your printer will use for bridges. "
                   "Set zero to disable acceleration control for bridges.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("bridge_angle", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Bridging angle");
    def->category = L("Infill");
    def->tooltip = L("Bridging angle override. If left to zero, the bridging angle will be calculated "
                   "automatically. Otherwise the provided angle will be used for all bridges. "
                   "Use 180° for zero angle.");
    def->sidetext = L("°");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("bridge_fan_speed", Int);
    def->location = "filament_settings";
    def->label = L("Bridges fan speed");
    def->tooltip = L("This fan speed is enforced during all bridges and overhangs.");
    def->sidetext = L("%");
    def->min = 0;
    def->max = 100;
    def->mode = comExpert;
    SET_DEFAULT( 100 );

    def = defs.add("bridge_flow_ratio", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Bridge flow ratio");
    def->category = L("Advanced");
    def->tooltip = L("This factor affects the amount of plastic for bridging. "
                   "You can decrease it slightly to pull the extrudates and prevent sagging, "
                   "although default settings are usually good and you should experiment "
                   "with cooling (use a fan) before tweaking this.");
    def->min = 0.;
    def->max = 2.;
    def->mode = comAdvanced;
    SET_DEFAULT(1.);

    def = defs.add("top_one_perimeter_type", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Single perimeter on top surfaces");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Use only one perimeter on flat top surface, to give more space to the top infill pattern. Could be applied on topmost surface or all top surfaces.");
    def->mode = comExpert;
    def->enum_type = TopOnePerimeterType::None;
    def->enum_values = { { int(TopOnePerimeterType::None), "none", L("Disabled") },
                         { int(TopOnePerimeterType::TopSurfaces), "top", L("All top surfaces") },
                         { int(TopOnePerimeterType::TopmostOnly), "topmost", L("Topmost surface only") } };
    SET_DEFAULT(TopOnePerimeterType::None);

    def = defs.add("only_one_perimeter_first_layer", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Only one perimeter on first layer");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Use only one perimeter on the first layer.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("bridge_speed", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Bridges");
    def->category = L("Speed");
    def->tooltip = L("Speed for printing bridges.");
    def->sidetext = L("mm/s");
    def->aliases = { "bridge_feed_rate" };
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(60.);

    def = defs.add("over_bridge_speed", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    // TRN: Label for speed used to print infill above bridges.
    def->label = L("Over bridges");
    def->category = L("Speed");
    def->tooltip = L("Speed for printing solid infill above bridges. Set to 0 to use solid infill speed. "
                    "If set as percentage, the speed is calculated over solid infill speed. ");
    def->sidetext = L("mm/s or %");;
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage{0.});

    def             = defs.add("enable_dynamic_overhang_speeds", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label      = L("Enable dynamic overhang speeds");
    def->category   = L("Speed");
    def->tooltip    = L("This setting enables dynamic speed control on overhangs.");
    def->mode       = comExpert;
    SET_DEFAULT(false);

    // TRN PrintSettings : "Dynamic overhang speed"
    auto overhang_speed_setting_description = L("Overhang size is expressed as a percentage of overlap of the extrusion with the previous layer: "
                        "100% would be full overlap (no overhang), while 0% represents full overhang (floating extrusion, bridge). "
                        "Speeds for overhang sizes in between are calculated via linear interpolation. "
                        "If set as percentage, the speed is calculated over the external perimeter speed. "
                        "Note that the speeds generated to gcode will never exceed the max volumetric speed value.");

    def             = defs.add("overhang_speed_0", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label      = L("speed for 0% overlap (bridge)");
    def->category   = L("Speed");
    def->tooltip    = overhang_speed_setting_description;
    def->sidetext   = L("mm/s or %");
    def->min        = 0;
    def->mode       = comExpert;
    SET_DEFAULT(FloatOrPercentage{15.});

    def             = defs.add("overhang_speed_1", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label      = L("speed for 25% overlap");
    def->category   = L("Speed");
    def->tooltip    = overhang_speed_setting_description;
    def->sidetext   = L("mm/s or %");
    def->min        = 0;
    def->mode       = comExpert;
    SET_DEFAULT(FloatOrPercentage{15.});

    def             = defs.add("overhang_speed_2", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label      = L("speed for 50% overlap");
    def->category   = L("Speed");
    def->tooltip    = overhang_speed_setting_description;
    def->sidetext   = L("mm/s or %");
    def->min        = 0;
    def->mode       = comExpert;
    SET_DEFAULT(FloatOrPercentage{20.});

    def             = defs.add("overhang_speed_3", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label      = L("speed for 75% overlap");
    def->category   = L("Speed");
    def->tooltip    = overhang_speed_setting_description;
    def->sidetext   = L("mm/s or %");
    def->min        = 0;
    def->mode       = comExpert;
    SET_DEFAULT(FloatOrPercentage{25.});

    def          = defs.add("enable_dynamic_fan_speeds", Bool);
    def->location = "filament_settings";
    def->label   = L("Enable dynamic fan speeds");
    def->tooltip = L("This setting enables dynamic fan speed control on overhangs.");
    def->mode    = comExpert;
    SET_DEFAULT(false);

    // TRN FilamentSettings : "Dynamic fan speeds"
    auto fan_speed_setting_description = L("Overhang size is expressed as a percentage of overlap of the extrusion with the previous layer: "
        "100% would be full overlap (no overhang), while 0% represents full overhang (floating extrusion, bridge). "
        "Fan speeds for overhang sizes in between are calculated via linear interpolation.");

    def           = defs.add("overhang_fan_speed_0", Int);
    def->location = "filament_settings";
    def->label    = L("speed for 0% overlap (bridge)");
    def->tooltip  = fan_speed_setting_description;
    def->sidetext = L("%");
    def->min      = 0;
    def->max      = 100;
    def->mode     = comExpert;
    SET_DEFAULT(0);

    def           = defs.add("overhang_fan_speed_1", Int);
    def->location = "filament_settings";
    def->label    = L("speed for 25% overlap");
    def->tooltip  = fan_speed_setting_description;
    def->sidetext = L("%");
    def->min      = 0;
    def->max      = 100;
    def->mode     = comExpert;
    SET_DEFAULT(0);

    def           = defs.add("overhang_fan_speed_2", Int);
    def->location = "filament_settings";
    def->label    = L("speed for 50% overlap");
    def->tooltip  = fan_speed_setting_description;
    def->sidetext = L("%");
    def->min      = 0;
    def->max      = 100;
    def->mode     = comExpert;
    SET_DEFAULT(0);

    def           = defs.add("overhang_fan_speed_3", Int);
    def->location = "filament_settings";
    def->label    = L("speed for 75% overlap");
    def->tooltip  = fan_speed_setting_description;
    def->sidetext = L("%");
    def->min      = 0;
    def->max      = 100;
    def->mode     = comExpert;
    SET_DEFAULT(0);

    def = defs.add("brim_width", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Brim width");
    def->category = L("Skirt and brim");
    def->tooltip = L("The horizontal width of the brim that will be printed around each object on the first layer. "
                     "When raft is used, no brim is generated (use raft_first_layer_expansion).");
    def->sidetext = L("mm");
    def->min = 0;
    def->max = 200;
    def->mode = comSimple;
    SET_DEFAULT(0.);

    def = defs.add("brim_type", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Brim type");
    def->category = L("Skirt and brim");
    def->tooltip = L("The places where the brim will be printed around each object on the first layer.");
    def->enum_type = BrimType::NoBrim;
    def->enum_values = { { int(BrimType::NoBrim),        "no_brim",         L("No brim") },
                         { int(BrimType::OuterOnly),     "outer_only",      L("Outer brim only") },
                         { int(BrimType::InnerOnly),     "inner_only",      L("Inner brim only") },
                         { int(BrimType::OuterAndInner), "outer_and_inner", L("Outer and inner brim") } };
    def->mode = comSimple;
    SET_DEFAULT(BrimType::OuterOnly);

    def = defs.add("brim_separation", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Brim separation gap");
    def->category = L("Skirt and brim");
    def->tooltip = L("Offset of brim from the printed object. The offset is applied after the elephant foot compensation.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    /* TODO: Isn't this one legacy? Doesn't the legacy loader remove it ?
    * It is part of s_project_options...

    def = defs.add("colorprint_heights", Doubles);
    def->label = L("Colorprint height");
    def->tooltip = L("Heights at which a filament change is to occur.");
    def->init_fn = [](ConfigItem& item) { item.vec<double>() = {}; };*/

    /* TODO: How to handle this crap?
    def = defs.add("compatible_printers", Strings);
    def->label = L("Compatible printers");
    def->mode = comAdvanced;
    SET_DEFAULT( new ConfigOptionStrings());
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("compatible_printers_condition", String);
    def->label = L("Compatible printers condition");
    def->tooltip = L("A boolean expression using the configuration values of an active printer profile. "
                   "If this expression evaluates to true, this profile is considered compatible "
                   "with the active printer profile.");
    def->mode = comExpert;
    SET_DEFAULT( new ConfigOptionString());
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("compatible_prints", Strings);
    def->label = L("Compatible print profiles");
    def->mode = comAdvanced;
    SET_DEFAULT( new ConfigOptionStrings());
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("compatible_prints_condition", String);
    def->label = L("Compatible print profiles condition");
    def->tooltip = L("A boolean expression using the configuration values of an active print profile. "
                   "If this expression evaluates to true, this profile is considered compatible "
                   "with the active print profile.");
    def->mode = comExpert;
    SET_DEFAULT( new ConfigOptionString());
    def->cli = ConfigOptionDef::nocli;

    // The following value is to be stored into the project file (AMF, 3MF, Config ...)
    // and it contains a sum of "compatible_printers_condition" values over the print and filament profiles.
    def = defs.add("compatible_printers_condition_cummulative", Strings);
    SET_DEFAULT( new ConfigOptionStrings());
    def->cli = ConfigOptionDef::nocli;
    def = defs.add("compatible_prints_condition_cummulative", Strings);
    SET_DEFAULT( new ConfigOptionStrings());
    def->cli = ConfigOptionDef::nocli;*/

    def = defs.add("complete_objects", Bool);
    def->location = "print_settings";
    def->label = L("Complete individual objects");
    def->tooltip = L("When printing multiple objects or copies, this feature will complete "
                   "each object before moving onto next one (and starting it from its bottom layer). "
                   "This feature is useful to avoid the risk of ruined prints. "
                   "Slic3r should warn and prevent you from extruder collisions, but beware.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("cooling", Bool);
    def->location = "filament_settings";
    def->label = L("Enable auto cooling");
    def->tooltip = L("This flag enables the automatic cooling logic that adjusts print speed "
                   "and fan speed according to layer printing time.");
    SET_DEFAULT(true);

    def = defs.add("cooling_tube_retraction", Double);
    def->location = "printer_settings";
    def->label = L("Cooling tube position");
    def->tooltip = L("Distance of the center-point of the cooling tube from the extruder tip.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(91.5);

    def = defs.add("cooling_tube_length", Double);
    def->location = "printer_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Cooling tube length");
    def->tooltip = L("Length of the cooling tube to limit space for cooling moves inside it.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(5.);

    def = defs.add("default_acceleration", Double);
    def->location = "print_settings";
    def->label = L("Default");
    def->tooltip = L("This is the acceleration your printer will be reset to after "
                   "the role-specific acceleration values are used (perimeter/infill). "
                   "Set zero to prevent resetting acceleration at all.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    /* TODO: What about these?
    def = defs.add("default_filament_profile", Strings);
    def->label = L("Default filament profile");
    def->tooltip = L("Default filament profile associated with the current printer profile. "
                   "On selection of the current printer profile, this filament profile will be activated.");
    SET_DEFAULT( new ConfigOptionStrings());
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("default_print_profile", String);
    def->label = L("Default print profile");
    def->tooltip = L("Default print profile associated with the current printer profile. "
                   "On selection of the current printer profile, this print profile will be activated.");
    SET_DEFAULT( new ConfigOptionString());
    def->cli = ConfigOptionDef::nocli;*/

    def = defs.add("disable_fan_first_layers", Int);
    def->location = "filament_settings";
    def->label = L("Disable fan for the first");
    def->tooltip = L("You can set this to a positive value to disable fan at all "
                   "during the first layers, so that it does not make adhesion worse.");
    def->sidetext = L("layers");
    def->min = 0;
    def->max = 1000;
    def->mode = comExpert;
    SET_DEFAULT(3);

    def = defs.add("dont_support_bridges", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Don't support bridges");
    def->category = L("Support material");
    def->tooltip = L("Experimental option for preventing support material from being generated "
                   "under bridged areas.");
    def->mode = comAdvanced;
    SET_DEFAULT(true);

    /* TODO: What is this?
    def = defs.add("duplicate_distance", Double);
    def->label = L("Distance between copies");
    def->tooltip = L("Distance used for the auto-arrange feature of the plater.");
    def->sidetext = L("mm");
    def->aliases = { "multiply_distance" };
    def->min = 0;
    SET_DEFAULT(6);*/

    def = defs.add("end_gcode", String);
    def->location = "printer_settings";
    def->label = L("End G-code");
    def->tooltip = L("This end procedure is inserted at the end of the output file. "
                   "Note that you can use placeholder variables for all PrusaSlicer settings.");
    def->multiline = true;
    def->full_width = true;
    def->height = 12;
    def->mode = comExpert;
    SET_DEFAULT("M104 S0 ; turn off temperature\nG28 X0  ; home X axis\nM84     ; disable motors\n");



    /////////////////////////////////////////////////////



    
    def = defs.add("end_filament_gcode", String);
    def->location = "filament_settings";
    def->label = L("End G-code");
    def->tooltip = L("This end procedure is inserted at the end of the output file, before the printer end gcode (and "
                   "before any toolchange from this filament in case of multimaterial printers). "
                   "Note that you can use placeholder variables for all PrusaSlicer settings. "
                   "If you have multiple extruders, the gcode is processed in extruder order.");
    def->multiline = true;
    def->full_width = true;
    def->height = 120;
    def->mode = comExpert;
    SET_DEFAULT("; Filament-specific end gcode \n;END gcode for filament\n");

    def = defs.add("ensure_vertical_shell_thickness", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Ensure vertical shell thickness");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Add solid infill near sloping surfaces to guarantee the vertical shell thickness "
                   "(top+bottom solid layers).");
    def->enum_type = EnsureVerticalShellThickness::Disabled;
    def->enum_values = { { int(EnsureVerticalShellThickness::Disabled), "disabled", L("Disabled") },
                         { int(EnsureVerticalShellThickness::Partial), "partial",  L("Partial") },
                         { int(EnsureVerticalShellThickness::Enabled), "enabled",  L("Enabled") } };
    def->mode = comAdvanced;
    SET_DEFAULT(EnsureVerticalShellThickness::Enabled);

    auto def_top_fill_pattern = def = defs.add("top_fill_pattern", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Top fill pattern");
    def->category = L("Infill");
    def->tooltip = L("Fill pattern for top infill. This only affects the top visible layer, and not its adjacent solid shells.");
    def->cli = "top-fill-pattern|external-fill-pattern|solid-fill-pattern";
    def->enum_type = InfillPattern::ip3DHoneycomb;
    def->enum_values = { { int(InfillPattern::ipRectilinear),        "rectilinear",        L("Rectilinear") },
                         { int(InfillPattern::ipMonotonic),          "monotonic",          L("Monotonic") },
                         { int(InfillPattern::ipMonotonicLines),     "monotoniclines",     L("Monotonic Lines") },
                         { int(InfillPattern::ipAlignedRectilinear), "alignedrectilinear", L("Aligned Rectilinear") },
                         { int(InfillPattern::ipConcentric),         "concentric",         L("Concentric") },
                         { int(InfillPattern::ipHilbertCurve),       "hilbertcurve",       L("Hilbert Curve") },
                         { int(InfillPattern::ipArchimedeanChords),  "archimedeanchords",  L("Archimedean Chords") },
                         { int(InfillPattern::ipOctagramSpiral),     "octagramspiral",     L("Octagram Spiral") } };
    // solid_fill_pattern is an obsolete equivalent to top_fill_pattern/bottom_fill_pattern.
    def->aliases = { "solid_fill_pattern", "external_fill_pattern" };
    SET_DEFAULT(InfillPattern::ipMonotonic);

    def = defs.add("bottom_fill_pattern", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Bottom fill pattern");
    def->category = L("Infill");
    def->tooltip = L("Fill pattern for bottom infill. This only affects the bottom external visible layer, and not its adjacent solid shells.");
    def->cli = "bottom-fill-pattern|external-fill-pattern|solid-fill-pattern";
    def->enum_type = InfillPattern::ip3DHoneycomb;
    def->enum_values = { { int(InfillPattern::ipRectilinear),        "rectilinear",        L("Rectilinear") },
                         { int(InfillPattern::ipMonotonic),          "monotonic",          L("Monotonic") },
                         { int(InfillPattern::ipMonotonicLines),     "monotoniclines",     L("Monotonic Lines") },
                         { int(InfillPattern::ipAlignedRectilinear), "alignedrectilinear", L("Aligned Rectilinear") },
                         { int(InfillPattern::ipConcentric),         "concentric",         L("Concentric") },
                         { int(InfillPattern::ipHilbertCurve),       "hilbertcurve",       L("Hilbert Curve") },
                         { int(InfillPattern::ipArchimedeanChords),  "archimedeanchords",  L("Archimedean Chords") },
                         { int(InfillPattern::ipOctagramSpiral),     "octagramspiral",     L("Octagram Spiral") } };
    def->aliases = def_top_fill_pattern->aliases;
    SET_DEFAULT(InfillPattern::ipMonotonic);

    def = defs.add("external_perimeter_extrusion_width", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("External perimeters");
    def->category = L("Extrusion Width");
    def->tooltip = L("Set this to a non-zero value to set a manual extrusion width for external perimeters. "
                   "If left zero, default extrusion width will be used if set, otherwise 1.125 x nozzle diameter will be used. "
                   "If expressed as percentage (for example 200%), it will be computed over layer height.");
    def->sidetext = L("mm or %");
    def->min = 0;
    def->max_literal = 50;
    def->mode = comAdvanced;
    def->ratio_over = "layer_height";
    SET_DEFAULT(FloatOrPercentage{0.});

    def = defs.add("external_perimeter_speed", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("External perimeters");
    def->category = L("Speed");
    def->tooltip = L("This separate setting will affect the speed of external perimeters (the visible ones). "
                   "If expressed as percentage (for example: 80%) it will be calculated "
                   "on the perimeters speed setting above. Set to zero for auto.");
    def->sidetext = L("mm/s or %");
    def->ratio_over = "perimeter_speed";
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage(Percentage{50.}));

    def = defs.add("external_perimeters_first", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("External perimeters first");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Print contour perimeters from the outermost one to the innermost one "
                   "instead of the default inverse order.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("extra_perimeters", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Extra perimeters if needed");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Add more perimeters when needed for avoiding gaps in sloping walls. "
                   "Slic3r keeps adding perimeters, until more than 70% of the loop immediately above "
                   "is supported.");
    def->mode = comExpert;
    SET_DEFAULT(true);

    def = defs.add("extra_perimeters_on_overhangs", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Extra perimeters on overhangs (Experimental)");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Detect overhang areas where bridges cannot be anchored, and fill them with "
                    "extra perimeter paths. These paths are anchored to the nearby non-overhang area when possible.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("extruder", Int);
    def->location = "object_settings";
    def->overrides_in = { "volume_settings" };
    def->label = L("Extruder");
    def->category = L("Extruders");
    def->tooltip = L("The extruder to use (unless more specific extruder settings are specified). "
                   "This value overrides perimeter and infill extruders, but not the support extruders.");
    def->min = 0;  // 0 = inherit defaults
    def->choices = {
        { 0,  L("default") },
        { 1,  "1" },
        { 2,  "2" },
        { 3,  "3" },
        { 4,  "4" },
        { 5,  "5" }
    };
    SET_DEFAULT(0);
        
    def = defs.add("extruder_colour", String);
    def->location = "toolprint_settings";
    def->label = L("Extruder Color");
    def->tooltip = L("This is only used in the Slic3r interface as a visual help.");
    def->gui_type = ConfigItemDef::GUIType::color;
    SET_DEFAULT(""); // Empty string means no color assigned yet.

    def = defs.add("extruder_offset", Point);
    def->location = "toolprint_settings";
    def->label = L("Extruder offset");
    def->tooltip = L("If your firmware doesn't handle the extruder displacement you need the G-code "
                   "to take it into account. This option lets you specify the displacement of each extruder "
                   "with respect to the first one. It expects positive coordinates (they will be subtracted "
                   "from the XY coordinate).");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    SET_DEFAULT(Vec2d(0,0));

    /* TODO: shouldn't we remove this crap?
    def = defs.add("extrusion_axis", String);
    def->label = L("Extrusion axis");
    def->tooltip = L("Use this option to set the axis letter associated to your printer's extruder "
                   "(usually E but some printers use A).");
    SET_DEFAULT("E"));*/

    def = defs.add("extruder_clearance_height", Double);
    def->location = "print_settings";
    def->label = L("Height");
    def->tooltip = L("Only used when 'Print Settings -> Complete individual objects' is active. Set this to the vertical "
                   "distance between your nozzle tip and (usually) the X carriage rods. Used to check for collisions "
                   "with previously printed objects and to prevent them when arranging.\n"
                   "The value is ignored for most Prusa printers, which come with more detailed extruder model.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(20.);

    def = defs.add("extruder_clearance_radius", Double);
    def->location = "print_settings";
    def->label = L("Radius");
    def->tooltip = L("Only used when 'Print Settings -> Complete individual objects' is active. Set this to a radius "
                     "of a nozzle-centered cylinder big enough to enclose the extruder assembly. Used to check for collisions "
                     "with previously printed objects and to prevent them when arranging.\n"
                     "The value is ignored for most Prusa printers, which come with more detailed extruder model.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(20.);    

    def = defs.add("extrusion_multiplier", Double);
    def->location = "filament_settings";
    def->label = L("Extrusion multiplier");
    def->tooltip = L("This factor changes the amount of flow proportionally. You may need to tweak "
                   "this setting to get nice surface finish and correct single wall widths. "
                   "Usual values are between 0.9 and 1.1. If you think you need to change this more, "
                   "check filament diameter and your firmware E steps.");
    def->max = 2;
    def->mode = comAdvanced;
    SET_DEFAULT(1.);

    def = defs.add("extrusion_width", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Default extrusion width");
    def->category = L("Extrusion Width");
    def->tooltip = L("Set this to a non-zero value to allow a manual extrusion width. "
                   "If left to zero, Slic3r derives extrusion widths from the nozzle diameter "
                   "(see the tooltips for perimeter extrusion width, infill extrusion width etc). "
                   "If expressed as percentage (for example: 230%), it will be computed over layer height.");
    def->sidetext = L("mm or %");
    def->min = 0;
    def->max = 1000;
    def->max_literal = 50;
    def->mode = comAdvanced;
    def->ratio_over = "layer_height";
    SET_DEFAULT(FloatOrPercentage{0.});

    def = defs.add("fan_always_on", Bool);
    def->location = "filament_settings";
    def->label = L("Keep fan always on");
    def->tooltip = L("If this is enabled, fan will never be disabled and will be kept running at least "
                   "at its minimum speed. Useful for PLA, harmful for ABS.");
    SET_DEFAULT(false);

    def = defs.add("fan_below_layer_time", Int);
    def->location = "filament_settings";
    def->label = L("Enable fan if layer print time is below");
    def->tooltip = L("If layer print time is estimated below this number of seconds, fan will be enabled "
                   "and its speed will be calculated by interpolating the minimum and maximum speeds.");
    def->sidetext = L("approximate seconds");
    def->min = 0;
    def->max = 1000;
    def->mode = comExpert;
    SET_DEFAULT(60);

    def = defs.add("filament_colour", String);
    def->location = "filament_settings";
    def->label = L("Color");
    def->tooltip = L("This is only used in the Slic3r interface as a visual help.");
    def->gui_type = ConfigItemDef::GUIType::color;
    SET_DEFAULT("#29B2B2");

    def = defs.add("filament_notes", String);
    def->location = "filament_settings";
    def->label = L("Filament notes");
    def->tooltip = L("You can put your notes regarding the filament here.");
    def->multiline = true;
    def->full_width = true;
    def->height = 13;
    def->mode = comAdvanced;
    SET_DEFAULT("");

    def = defs.add("filament_max_volumetric_speed", Double);
    def->location = "filament_settings";
    def->label = L("Max volumetric speed");
    def->tooltip = L("Maximum volumetric speed allowed for this filament. Limits the maximum volumetric "
                   "speed of a print to the minimum of print and filament volumetric speed. "
                   "Set to zero for no limit.");
    def->sidetext = L("mm³/s");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("filament_infill_max_speed", Double);
    def->location = "filament_settings";
    def->label = L("Max non-crossing infill speed");
    def->tooltip = L("Maximum speed allowed for this filament while printing infill without "
                     "any self intersections in a single layer. "
                     "Set to zero for no limit.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("filament_infill_max_crossing_speed", Double);
    def->location = "filament_settings";
    def->label = L("Max crossing infill speed");
    def->tooltip = L("Maximum speed allowed for this filament while printing infill with "
                     "self intersections in a single layer. "
                     "Set to zero for no limit.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("filament_loading_speed", Double);
    def->location = "filament_settings";
    def->label = L("Loading speed");
    def->tooltip = L("Speed used for loading the filament on the wipe tower.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(28.);

    def = defs.add("filament_loading_speed_start", Double);
    def->location = "filament_settings";
    def->label = L("Loading speed at the start");
    def->tooltip = L("Speed used at the very beginning of loading phase.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(3.);

    def = defs.add("filament_unloading_speed", Double);
    def->location = "filament_settings";
    def->label = L("Unloading speed");
    def->tooltip = L("Speed used for unloading the filament on the wipe tower (does not affect "
                      " initial part of unloading just after ramming).");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(90.);

    def = defs.add("filament_unloading_speed_start", Double);
    def->location = "filament_settings";
    def->label = L("Unloading speed at the start");
    def->tooltip = L("Speed used for unloading the tip of the filament immediately after ramming.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(100.);

    def = defs.add("filament_toolchange_delay", Double);
    def->location = "filament_settings";
    def->label = L("Delay after unloading");
    def->tooltip = L("Time to wait after the filament is unloaded. "
                   "May help to get reliable toolchanges with flexible materials "
                   "that may need more time to shrink to original dimensions.");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("filament_stamping_loading_speed", Double);
    def->location = "filament_settings";
    def->label = L("Stamping loading speed");
    def->tooltip = L("Speed used for stamping.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(20.);

    def = defs.add("filament_stamping_distance", Double);
    def->location = "filament_settings";
    def->label = L("Stamping distance measured from the center of the cooling tube");
    def->tooltip = L("If set to nonzero value, filament is moved toward the nozzle between the individual cooling moves (\"stamping\"). "
                     "This option configures how long this movement should be before the filament is retracted again.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("filament_cooling_moves", Int);
    def->location = "filament_settings";
    def->label = L("Number of cooling moves");
    def->tooltip = L("Filament is cooled by being moved back and forth in the "
                   "cooling tubes. Specify desired number of these moves.");
    def->max = 0;
    def->max = 20;
    def->mode = comExpert;
    SET_DEFAULT(4);

    def = defs.add("filament_cooling_initial_speed", Double);
    def->location = "filament_settings";
    def->label = L("Speed of the first cooling move");
    def->tooltip = L("Cooling moves are gradually accelerating beginning at this speed.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(2.2);

    def = defs.add("filament_minimal_purge_on_wipe_tower", Double);
    def->location = "filament_settings";
    def->label = L("Minimal purge on wipe tower");
    def->tooltip = L("After a tool change, the exact position of the newly loaded filament inside "
                     "the nozzle may not be known, and the filament pressure is likely not yet stable. "
                     "Before purging the print head into an infill or a sacrificial object, Slic3r will always prime "
                     "this amount of material into the wipe tower to produce successive infill or sacrificial object extrusions reliably.");
    def->sidetext = L("mm³");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(15.);

    def = defs.add("filament_cooling_final_speed", Double);
    def->location = "filament_settings";
    def->label = L("Speed of the last cooling move");
    def->tooltip = L("Cooling moves are gradually accelerating towards this speed.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(3.4);

    def = defs.add("filament_purge_multiplier", Percent);
    def->location = "filament_settings";
    def->label = L("Purge volume multiplier");
    def->tooltip = L("Purging volume on the wipe tower is determined by 'multimaterial_purging' in Printer Settings. "
                     "This option allows to modify the volume on filament level. "
                     "Note that the project can override this by setting project-specific values.");
    def->sidetext = L("%");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(Percentage{100.});

    def = defs.add("filament_load_time", Double);
    def->location = "filament_settings";
    def->label = L("Filament load time");
    def->tooltip = L("Time for the printer firmware (or the Multi Material Unit 2.0) to load a new filament during a tool change (when executing the T code). This time is added to the total print time by the G-code time estimator.");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("filament_ramming_parameters", String);
    def->location = "filament_settings";
    def->label = L("Ramming parameters");
    def->tooltip = L("This string is edited by RammingDialog and contains ramming specific parameters.");
    def->mode = comExpert;
    SET_DEFAULT("120 100 6.6 6.8 7.2 7.6 7.9 8.2 8.7 9.4 9.9 10.0|"
       " 0.05 6.6 0.45 6.8 0.95 7.8 1.45 8.3 1.95 9.7 2.45 10 2.95 7.6 3.45 7.6 3.95 7.6 4.45 7.6 4.95 7.6");

    def = defs.add("filament_unload_time", Double);
    def->location = "filament_settings";
    def->label = L("Filament unload time");
    def->tooltip = L("Time for the printer firmware (or the Multi Material Unit 2.0) to unload a filament during a tool change (when executing the T code). This time is added to the total print time by the G-code time estimator.");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("filament_multitool_ramming", Bool);
    def->location = "filament_settings";
    def->label = L("Enable ramming for multitool setups");
    def->tooltip = L("Perform ramming when using multitool printer (i.e. when the 'Single Extruder Multimaterial' in Printer Settings is unchecked). "
                     "When checked, a small amount of filament is rapidly extruded on the wipe tower just before the toolchange. "
                     "This option is only used when the wipe tower is enabled.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("filament_multitool_ramming_volume", Double);
    def->location = "filament_settings";
    def->label = L("Multitool ramming volume");
    def->tooltip = L("The volume to be rammed before the toolchange.");
    def->sidetext = L("mm³");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(10.);

    def = defs.add("filament_multitool_ramming_flow", Double);
    def->location = "filament_settings";
    def->label = L("Multitool ramming flow");
    def->tooltip = L("Flow used for ramming the filament before the toolchange.");
    def->sidetext = L("mm³/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(10.);

    def = defs.add("filament_diameter", Double);
    def->location = "filament_settings";
    def->label = L("Diameter");
    def->tooltip = L("Enter your filament diameter here. Good precision is required, so use a caliper "
                   "and do multiple measurements along the filament, then compute the average.");
    def->sidetext = L("mm");
    def->min = 0;
    SET_DEFAULT(1.75);

    def = defs.add("filament_density", Double);
    def->location = "filament_settings";
    def->label = L("Density");
    def->tooltip = L("Enter your filament density here. This is only for statistical information. "
                   "A decent way is to weigh a known length of filament and compute the ratio "
                   "of the length to volume. Better is to calculate the volume directly through displacement.");
    def->sidetext = L("g/cm³");
    def->min = 0;
    SET_DEFAULT(0.);

    def = defs.add("filament_type", String);
    def->location = "filament_settings";
    def->label = L("Filament type");
    def->tooltip = L("The filament material type for use in custom G-codes.");
    def->gui_flags = "show_value";
    def->choices = {
        { std::string("PLA"),  std::string("PLA")   },
        { std::string("PET"),  std::string("PET")   },
        { std::string("ABS"),  std::string("ABS")   },
        { std::string("ASA"),  std::string("ASA")   },
        { std::string("FLEX"), std::string("FLEX")  },
        { std::string("HIPS"), std::string("HIPS")  },
        { std::string("EDGE"), std::string("EDGE")  },
        { std::string("NGEN"), std::string("NGEN")  },
        { std::string("PA"),   std::string("PA")    },
        { std::string("NYLON"),std::string("NYLON") },
        { std::string("PVA"),  std::string("PVA")   },
        { std::string("PC"),   std::string("PC")    },
        { std::string("PP"),   std::string("PP")    },
        { std::string("PEI"),  std::string("PEI")   },
        { std::string("PEEK"), std::string("PEEK")  },
        { std::string("PEKK"), std::string("PEKK")  },
        { std::string("POM"),  std::string("POM")   },
        { std::string("PSU"),  std::string("PSU")   },
        { std::string("PVDF"), std::string("PVDF")  },
        { std::string("SCAFF"),std::string("SCAFF") },
    };
    def->mode = comAdvanced;
    SET_DEFAULT("PLA");

    def = defs.add("filament_soluble", Bool);
    def->location = "filament_settings";
    def->label = L("Soluble material");
    def->tooltip = L("Soluble material is most likely used for a soluble support.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("filament_abrasive", Bool);
    def->location = "filament_settings";
    def->label = L("Abrasive material");
    def->tooltip = L("This flag means that the material is abrasive and requires a hardened nozzle. The value is used by the printer to check it.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("filament_cost", Double);
    def->location = "filament_settings";
    def->label = L("Cost");
    def->tooltip = L("Enter your filament cost per kg here. This is only for statistical information.");
    def->sidetext = L("money/kg");
    def->min = 0;
    SET_DEFAULT(0.);

    def = defs.add("filament_spool_weight", Double);
    def->location = "filament_settings";
    def->label = L("Spool weight");
    def->tooltip = L("Enter weight of the empty filament spool. "
                     "One may weigh a partially consumed filament spool before printing and one may compare the measured weight "
                     "with the calculated weight of the filament with the spool to find out whether the amount "
                     "of filament on the spool is sufficient to finish the print.");
    def->sidetext = L("g");
    def->min = 0;
    SET_DEFAULT(0.);

    /* TODO: What about this?
    def = defs.add("filament_settings_id", Strings);
    SET_DEFAULT( new ConfigOptionStrings { "" });
    def->cli = ConfigOptionDef::nocli;*/

    def = defs.add("filament_vendor", String);
    def->location = "filament_settings";
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT(L("(Unknown)"));

    def = defs.add("filament_shrinkage_compensation_xy", Percent);
    def->location = "filament_settings";
    def->label = L("Shrinkage compensation XY");
    def->tooltip = L("Enter your filament shrinkage percentages for the X and Y axes here to apply scaling of the object to "
                     "compensate for shrinkage in the X and Y axes. For example, if you measured 99mm instead of 100mm, "
                     "enter 1%.");
    def->sidetext = L("%");
    def->mode = comAdvanced;
    def->min = -10.;
    def->max = 10.;
    SET_DEFAULT(Percentage{0.});

    def = defs.add("filament_shrinkage_compensation_z", Percent);
    def->location = "filament_settings";
    def->label = L("Shrinkage compensation Z");
    def->tooltip = L("Enter your filament shrinkage percentages for the Z axis here to apply scaling of the object to "
                     "compensate for shrinkage in the Z axis. For example, if you measured 99mm instead of 100mm, "
                     "enter 1%.");
    def->sidetext = L("%");
    def->mode = comAdvanced;
    def->min = -10.;
    def->max = 10.;
    SET_DEFAULT(Percentage{0.});

    def = defs.add("fill_angle", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Fill angle");
    def->category = L("Infill");
    def->tooltip = L("Default base angle for infill orientation. Cross-hatching will be applied to this. "
                   "Bridges will be infilled using the best direction Slic3r can detect, so this setting "
                   "does not affect them.");
    def->sidetext = L("°");
    def->min = 0;
    def->max = 360;
    def->mode = comAdvanced;
    SET_DEFAULT(45.);

    def = defs.add("fill_density", Percent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->gui_flags = "show_value";
    def->label = L("Fill density");
    def->category = L("Infill");
    def->tooltip = L("Density of internal infill, expressed in the range 0% - 100%.");
    def->sidetext = L("%");
    def->min = 0;
    def->max = 100;
    def->choices = {
        { 0., "0%" },
        { 5., "5%" },
        { 10., "10%" },
        { 15., "15%" },
        { 20., "20%" },
        { 25., "25%" },
        { 30., "30%" },
        { 40., "40%" },
        { 50., "50%" },
        { 60., "60%" },
        { 70., "70%" },
        { 80., "80%" },
        { 90., "90%" },
        { 100., "100%" }
    };
    SET_DEFAULT(Percentage{20.});

    def = defs.add("fill_pattern", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Fill pattern");
    def->category = L("Infill");
    def->tooltip = L("Fill pattern for general low-density infill.");
    def->enum_type = InfillPattern::ip3DHoneycomb;
    def->enum_values = {
        { int(InfillPattern::ipRectilinear),        "rectilinear",        L("Rectilinear") },
        { int(InfillPattern::ipAlignedRectilinear), "alignedrectilinear", L("Aligned Rectilinear") },
        { int(InfillPattern::ipGrid),               "grid",               L("Grid") },
        { int(InfillPattern::ipTriangles),          "triangles",          L("Triangles") },
        { int(InfillPattern::ipStars),              "stars",              L("Stars") },
        { int(InfillPattern::ipCubic),              "cubic",              L("Cubic") },
        { int(InfillPattern::ipLine),               "line",               L("Line") },
        { int(InfillPattern::ipConcentric),         "concentric",         L("Concentric") },
        { int(InfillPattern::ipHoneycomb),          "honeycomb",          L("Honeycomb") },
        { int(InfillPattern::ip3DHoneycomb),        "3dhoneycomb",        L("3D Honeycomb") },
        { int(InfillPattern::ipGyroid),             "gyroid",             L("Gyroid") },
        { int(InfillPattern::ipHilbertCurve),       "hilbertcurve",       L("Hilbert Curve") },
        { int(InfillPattern::ipArchimedeanChords),  "archimedeanchords",  L("Archimedean Chords") },
        { int(InfillPattern::ipOctagramSpiral),     "octagramspiral",     L("Octagram Spiral") },
        { int(InfillPattern::ipAdaptiveCubic),      "adaptivecubic",      L("Adaptive Cubic") },
        { int(InfillPattern::ipSupportCubic),       "supportcubic",       L("Support Cubic") },
        { int(InfillPattern::ipLightning),          "lightning",          L("Lightning") },
        { int(InfillPattern::ipZigZag),             "zigzag",             L("Zig Zag") } };
    SET_DEFAULT(InfillPattern::ipStars);

    def = defs.add("first_layer_acceleration", Double);
    def->location = "print_settings";
    def->label = L("First layer");
    def->tooltip = L("This is the acceleration your printer will use for first layer. Set zero "
                   "to disable acceleration control for first layer.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("first_layer_acceleration_over_raft", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("First object layer over raft interface");
    def->tooltip = L("This is the acceleration your printer will use for first layer of object above raft interface. Set zero "
                   "to disable acceleration control for first layer of object above raft interface.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("first_layer_bed_temperature", Int);
    def->location = "filament_settings";
    def->label = L("First layer");
    def->full_label = L("First layer bed temperature");
    def->tooltip = L("Heated build plate temperature for the first layer. Set this to zero to disable "
                   "bed temperature control commands in the output.");
    def->sidetext = L("°C");
    def->max = 0;
    def->max = 300;
    SET_DEFAULT(0);

    def = defs.add("first_layer_extrusion_width", FloatOrPercent);
    def->location = "print_settings";
    def->label = L("First layer");
    def->category = L("Extrusion Width");
    def->tooltip = L("Set this to a non-zero value to set a manual extrusion width for first layer. "
                   "You can use this to force fatter extrudates for better adhesion. If expressed "
                   "as percentage (for example 120%) it will be computed over first layer height. "
                   "If set to zero, it will use the default extrusion width.");
    def->sidetext = L("mm or %");
    def->ratio_over = "first_layer_height";
    def->min = 0;
    def->max_literal = 50;
    def->mode = comAdvanced;
    def->ratio_over = "first_layer_layer_height";
    SET_DEFAULT(FloatOrPercentage(Percentage{200.}));

    def = defs.add("first_layer_height", FloatOrPercent);
    def->location = "print_settings";
    def->label = L("First layer height");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("When printing with very low layer heights, you might still want to print a thicker "
                   "bottom layer to improve adhesion and tolerance for non perfect build plates.");
    def->sidetext = L("mm");
    def->min = 0;
    def->ratio_over = "layer_height";
    SET_DEFAULT(FloatOrPercentage{0.35});

    def = defs.add("first_layer_speed", FloatOrPercent);
    def->location = "print_settings";
    def->label = L("First layer speed");
    def->tooltip = L("If expressed as absolute value in mm/s, this speed will be applied to all the print moves "
                   "of the first layer, regardless of their type. If expressed as a percentage "
                   "(for example: 40%) it will scale the default speeds.");
    def->sidetext = L("mm/s or %");
    def->min = 0;
    def->max_literal = 20;
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage{30.});

    def = defs.add("first_layer_infill_speed", FloatOrPercent);
    def->location = "print_settings";
    def->label = L("First layer solid infill speed");
    def->tooltip = L("If expressed as absolute value in mm/s, this speed will be applied to the solid infill print moves "
                   "of the first layer. If expressed as a percentage "
                   "(for example: 40%) it will be a percantage of the solid infill speed "
                   "(for example: 40% of the solid infill speed). "
                   "Note that 0 means that the \"First layer speed\" value will be used.");
    def->sidetext = L("mm/s or %");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage{0.});

    def = defs.add("first_layer_speed_over_raft", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Speed of object first layer over raft interface");
    def->tooltip = L("If expressed as absolute value in mm/s, this speed will be applied to all the print moves "
                   "of the first object layer above raft interface, regardless of their type. If expressed as a percentage "
                   "(for example: 40%) it will scale the default speeds.");
    def->sidetext = L("mm/s or %");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage{30.});

    def = defs.add("first_layer_temperature", Int);
    def->location = "filament_settings";
    def->label = L("First layer");
    def->full_label = L("First layer nozzle temperature");
    def->tooltip = L("Nozzle temperature for the first layer. If you want to control temperature manually "
                     "during print, set this to zero to disable temperature control commands in the output G-code.");
    def->sidetext = L("°C");
    def->min = 0;
    def->max = max_temp;
    SET_DEFAULT(200);

    def = defs.add("full_fan_speed_layer", Int);
    def->location = "filament_settings";
    def->label = L("Full fan speed at layer");
    def->tooltip = L("Fan speed will be ramped up linearly from zero at layer \"disable_fan_first_layers\" "
                   "to maximum at layer \"full_fan_speed_layer\". "
                   "\"full_fan_speed_layer\" will be ignored if lower than \"disable_fan_first_layers\", in which case "
                   "the fan will be running at maximum allowed speed at layer \"disable_fan_first_layers\" + 1.");
    def->min = 0;
    def->max = 1000;
    def->mode = comExpert;
    SET_DEFAULT(0);

    def = defs.add("fuzzy_skin", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Fuzzy Skin");
    def->category = L("Fuzzy Skin");
    def->tooltip = L("Fuzzy skin type.");
    def->enum_type = FuzzySkinType::None;
    def->enum_values = { { int(FuzzySkinType::None),     "none",     L("None") },
                         { int(FuzzySkinType::External), "external", L("Outside walls") },
                         { int(FuzzySkinType::All),      "all",      L("All walls") } };
    def->mode = comSimple;
    SET_DEFAULT(FuzzySkinType::None);

    def = defs.add("fuzzy_skin_thickness", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Fuzzy skin thickness");
    def->category = L("Fuzzy Skin");
    def->tooltip = L("The maximum distance that each skin point can be offset (both ways), "
                     "measured perpendicular to the perimeter wall.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.3);

    def = defs.add("fuzzy_skin_point_dist", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Fuzzy skin point distance");
    def->category = L("Fuzzy Skin");
    def->tooltip = L("Perimeters will be split into multiple segments by inserting Fuzzy skin points. "
                     "Lowering the Fuzzy skin point distance will increase the number of randomly offset points on the perimeter wall.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.8);

    def = defs.add("gap_fill_enabled", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Fill gaps");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Enables filling of gaps between perimeters and between the inner most perimeters and infill.");
    def->mode = comAdvanced;
    SET_DEFAULT(true);

    def = defs.add("gap_fill_speed", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Gap fill");
    def->category = L("Speed");
    def->tooltip = L("Speed for filling small gaps using short zigzag moves. Keep this reasonably low "
                   "to avoid too much shaking and resonance issues. Set zero to disable gaps filling.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(20.);

    def = defs.add("gcode_comments", Bool);
    def->location = "print_settings";
    def->label = L("Verbose G-code");
    def->tooltip = L("Enable this to get a commented G-code file, with each line explained by a descriptive text. "
                   "If you print from SD card, the additional weight of the file could make your firmware "
                   "slow down.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("gcode_flavor", Enum);
    def->location = "printer_settings";
    def->label = L("G-code flavor");
    def->tooltip = L("Some G/M-code commands, including temperature control and others, are not universal. "
                   "Set this option to your printer's firmware to get a compatible output. "
                   "The \"No extrusion\" flavor prevents PrusaSlicer from exporting any extrusion value at all.");
    def->enum_type = GCodeFlavor::gcfNoExtrusion;
    def->enum_values = {
        { int(GCodeFlavor::gcfRepRapSprinter), "reprap",         L("RepRap/Sprinter") },
        { int(GCodeFlavor::gcfRepRapFirmware), "reprapfirmware", L("RepRapFirmware") },
        { int(GCodeFlavor::gcfRepetier),       "repetier",       L("Repetier") },
        { int(GCodeFlavor::gcfTeacup),         "teacup",         L("Teacup") },
        { int(GCodeFlavor::gcfMakerWare),      "makerware",      L("MakerWare (MakerBot)") },
        { int(GCodeFlavor::gcfMarlinLegacy),   "marlin",         L("Marlin (legacy)") },
        { int(GCodeFlavor::gcfMarlinFirmware), "marlin2",        L("Marlin 2") },
        { int(GCodeFlavor::gcfKlipper),        "klipper",        L("Klipper") },
        { int(GCodeFlavor::gcfSailfish),       "sailfish",       L("Sailfish (MakerBot)") },
        { int(GCodeFlavor::gcfMach3),          "mach3",          L("Mach3/LinuxCNC") },
        { int(GCodeFlavor::gcfMachinekit),     "machinekit",     L("Machinekit") },
        { int(GCodeFlavor::gcfSmoothie),       "smoothie",       L("Smoothie") },
        { int(GCodeFlavor::gcfNoExtrusion),    "no-extrusion",   L("No extrusion") } };
    def->mode = comExpert;
    SET_DEFAULT(GCodeFlavor::gcfRepRapSprinter);

    def = defs.add("gcode_label_objects", Enum);
    def->location = "print_settings";
    def->label = L("Label objects");
    def->tooltip = L("Selects whether labels should be exported at object boundaries and in what format.\n"
                     "OctoPrint = comments to be consumed by OctoPrint CancelObject plugin.\n"
                     "Firmware = firmware specific G-code (it will be chosen based on firmware flavor and it can end up to be empty).\n\n"
                     "This settings is NOT compatible with Single Extruder Multi Material setup and Wipe into Object / Wipe into Infill.");
    def->enum_type = LabelObjectsStyle::Disabled;
    def->enum_values = { { int(LabelObjectsStyle::Disabled),  "disabled", L("Disabled") },
                         { int(LabelObjectsStyle::Octoprint), "octoprint", L("OctoPrint comments") },
                         { int(LabelObjectsStyle::Firmware),  "firmware", L("Firmware-specific") } };
    def->mode = comAdvanced;
    SET_DEFAULT(LabelObjectsStyle::Disabled);

    def = defs.add("gcode_substitutions", Strings);
    def->location = "print_settings";
    def->label = L("G-code substitutions");
    def->tooltip = L("Find / replace patterns in G-code lines and substitute them.");
    def->mode = comExpert;
    SET_DEFAULT(std::vector<std::string>{});

    def = defs.add("high_current_on_filament_swap", Bool);
    def->location = "printer_settings";
    def->label = L("High extruder current on filament swap");
    def->tooltip = L("It may be beneficial to increase the extruder motor current during the filament exchange"
                   " sequence to allow for rapid ramming feed rates and to overcome resistance when loading"
                   " a filament with an ugly shaped tip.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("infill_acceleration", Double);
    def->location = "print_settings";
    def->label = L("Infill");
    def->tooltip = L("This is the acceleration your printer will use for infill. Set zero to disable "
                     "acceleration control for infill.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("solid_infill_acceleration", Double);
    def->location = "print_settings";
    def->label = L("Solid infill");
    def->tooltip = L("This is the acceleration your printer will use for solid infill. Set zero to use "
                     "the value for infill.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("top_solid_infill_acceleration", Double);
    def->location = "print_settings";
    def->label = L("Top solid infill");
    def->tooltip = L("This is the acceleration your printer will use for top solid infill. Set zero to use "
                     "the value for solid infill.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("wipe_tower_acceleration", Double);
    def->location = "print_settings";
    def->label = L("Wipe tower");
    def->tooltip = L("This is the acceleration your printer will use for wipe tower. Set zero to disable "
                     "acceleration control for the wipe tower.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("travel_acceleration", Double);
    def->location = "print_settings";
    def->label = L("Travel");
    def->tooltip = L("This is the acceleration your printer will use for travel moves. Set zero to disable "
                     "acceleration control for travel.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("infill_every_layers", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Combine infill every");
    def->category = L("Infill");
    def->tooltip = L("This feature allows to combine infill and speed up your print by extruding thicker "
                   "infill layers while preserving thin perimeters, thus accuracy.");
    def->sidetext = L("layers");
    def->full_label = L("Combine infill every n layers");
    def->min = 1;
    def->mode = comAdvanced;
    SET_DEFAULT(1);

    def = defs.add("infill_anchor", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Length of the infill anchor");
    def->category = L("Advanced");
    def->tooltip = L("Connect an infill line to an internal perimeter with a short segment of an additional perimeter. "
                     "If expressed as percentage (example: 15%) it is calculated over infill extrusion width. "
                     "PrusaSlicer tries to connect two close infill lines to a short perimeter segment. If no such perimeter segment "
                     "shorter than infill_anchor_max is found, the infill line is connected to a perimeter segment at just one side "
                     "and the length of the perimeter segment taken is limited to this parameter, but no longer than anchor_length_max. "
                     "Set this parameter to zero to disable anchoring perimeters connected to a single infill line.");
    def->sidetext = L("mm or %");
    def->ratio_over = "infill_extrusion_width";
    def->max_literal = 1000;
    def->choices = {
        { 0.,      L("0 (no open anchors)") },
        { 1.,      L("1 mm") },
        { 2.,      L("2 mm") },
        { 5.,      L("5 mm") },
        { 10.,     L("10 mm") },
        { 1000.,   L("1000 (unlimited)") }
    };
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage(Percentage{600.}));
    const ConfigItemDef* def_infill_anchor_min = def;

    def = defs.add("infill_anchor_max", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Maximum length of the infill anchor");
    def->category    = def_infill_anchor_min->category;
    def->tooltip = L("Connect an infill line to an internal perimeter with a short segment of an additional perimeter. "
                     "If expressed as percentage (example: 15%) it is calculated over infill extrusion width. "
                     "PrusaSlicer tries to connect two close infill lines to a short perimeter segment. If no such perimeter segment "
                     "shorter than this parameter is found, the infill line is connected to a perimeter segment at just one side "
                     "and the length of the perimeter segment taken is limited to infill_anchor, but no longer than this parameter. "
                     "Set this parameter to zero to disable anchoring.");
    def->sidetext    = def_infill_anchor_min->sidetext;
    def->ratio_over  = def_infill_anchor_min->ratio_over;
    def->max_literal = def_infill_anchor_min->max_literal;
    def->choices = {
        { 0.,      L("0 (not anchored)") },
        { 1.,      L("1 mm") },
        { 2.,      L("2 mm") },
        { 5.,      L("5 mm") },
        { 10.,     L("10 mm") },
        { 1000.,   L("1000 (unlimited)") }
    };
    def->mode        = def_infill_anchor_min->mode;
    SET_DEFAULT(FloatOrPercentage{50.});

    def = defs.add("infill_extruder", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Infill extruder");
    def->category = L("Extruders");
    def->tooltip = L("The extruder to use when printing infill.");
    def->min = 1;
    def->mode = comAdvanced;
    SET_DEFAULT(1);

    def = defs.add("infill_extrusion_width", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Infill");
    def->category = L("Extrusion Width");
    def->tooltip = L("Set this to a non-zero value to set a manual extrusion width for infill. "
                   "If left zero, default extrusion width will be used if set, otherwise 1.125 x nozzle diameter will be used. "
                   "You may want to use fatter extrudates to speed up the infill and make your parts stronger. "
                   "If expressed as percentage (for example 90%) it will be computed over layer height.");
    def->sidetext = L("mm or %");
    def->min = 0;
    def->max_literal = 50;
    def->mode = comAdvanced;
    def->ratio_over = "layer_height";
    SET_DEFAULT(FloatOrPercentage{0.});

    def = defs.add("infill_first", Bool);
    def->location = "print_settings";
    def->label = L("Infill before perimeters");
    def->tooltip = L("This option will switch the print order of perimeters and infill, making the latter first.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("infill_overlap", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Infill/perimeters overlap");
    def->category = L("Advanced");
    def->tooltip = L("This setting applies an additional overlap between infill and perimeters for better bonding. "
                   "Theoretically this shouldn't be needed, but backlash might cause gaps. If expressed "
                   "as percentage (example: 15%) it is calculated over perimeter extrusion width.");
    def->sidetext = L("mm or %");
    def->ratio_over = "perimeter_extrusion_width";
    def->mode = comExpert;
    SET_DEFAULT(FloatOrPercentage(Percentage{25.}));

    def = defs.add("infill_speed", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Infill");
    def->category = L("Speed");
    def->tooltip = L("Speed for printing the internal fill. Set to zero for auto.");
    def->sidetext = L("mm/s");
    def->aliases = { "print_feed_rate", "infill_feed_rate" };
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(80.);

    /* TODO: Co s timhle?
    def = defs.add("inherits", String);
    def->label = L("Inherits profile");
    def->tooltip = L("Name of the profile, from which this profile inherits.");
    def->full_width = true;
    def->height = 5;
    SET_DEFAULT( new ConfigOptionString());
    def->cli = ConfigOptionDef::nocli;

    // The following value is to be stored into the project file (AMF, 3MF, Config ...)
    // and it contains a sum of "inherits" values over the print and filament profiles.
    def = defs.add("inherits_cummulative", Strings);
    SET_DEFAULT( new ConfigOptionStrings());
    def->cli = ConfigOptionDef::nocli;*/

    def = defs.add("interface_shells", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Interface shells");
    def->tooltip = L("Force the generation of solid shells between adjacent materials/volumes. "
                   "Useful for multi-extruder prints with translucent materials or manual soluble "
                   "support material.");
    def->category = L("Layers and Perimeters");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("mmu_segmented_region_max_width", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Maximum width of a segmented region");
    def->tooltip = L("Maximum width of a segmented region. Zero disables this feature.");
    def->sidetext = L("mm (zero to disable)");
    def->min = 0;
    def->category = L("Advanced");
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("mmu_segmented_region_interlocking_depth", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Interlocking depth of a segmented region");
    def->tooltip = L("Interlocking depth of a segmented region. It will be ignored if "
                       "\"mmu_segmented_region_max_width\" is zero or if \"mmu_segmented_region_interlocking_depth\""
                       "is bigger then \"mmu_segmented_region_max_width\". Zero disables this feature.");
    def->sidetext = L("mm (zero to disable)");
    def->min = 0;
    def->category = L("Advanced");
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("ironing", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Enable ironing");
    def->tooltip = L("Enable ironing of the top layers with the hot print head for smooth surface");
    def->category = L("Ironing");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def           = defs.add("interlocking_beam", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label    = L("Use beam interlocking");
    def->tooltip  = L("Generate interlocking beam structure at the locations where different filaments touch. This improves the adhesion between filaments, especially models printed in different materials.");
    def->category = L("Advanced");
    def->mode     = comAdvanced;
    SET_DEFAULT(false);

    def           = defs.add("interlocking_beam_width", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label    = L("Interlocking beam width");
    def->tooltip  = L("The width of the interlocking structure beams.");
    def->sidetext = L("mm");
    def->min      = 0.1f;
    def->category = L("Advanced");
    def->mode     = comAdvanced;
    SET_DEFAULT(0.8);

    def           = defs.add("interlocking_orientation", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label    = L("Interlocking direction");
    def->tooltip  = L("Orientation of interlocking beams.");
    def->sidetext = L("°");
    def->min      = 0;
    def->max      = 360;
    def->category = L("Advanced");
    def->mode     = comAdvanced;
    SET_DEFAULT(22.5);

    def           = defs.add("interlocking_beam_layer_count", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label    = L("Interlocking beam layers");
    def->tooltip  = L("The height of the beams of the interlocking structure, measured in number of layers. Less layers is stronger, but more prone to defects.");
    def->min      = 1;
    def->category = L("Advanced");
    def->mode     = comAdvanced;
    SET_DEFAULT(2);

    def           = defs.add("interlocking_depth", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label    = L("Interlocking depth");
    def->tooltip  = L("The distance from the boundary between filaments to generate interlocking structure, measured in cells. Too few cells will result in poor adhesion.");
    def->min      = 1;
    def->category = L("Advanced");
    def->mode     = comAdvanced;
    SET_DEFAULT(2);

    def           = defs.add("interlocking_boundary_avoidance", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label    = L("Interlocking boundary avoidance");
    def->tooltip  = L("The distance from the outside of a model where interlocking structures will not be generated, measured in cells.");
    def->min      = 0;
    def->category = L("Advanced");
    def->mode     = comAdvanced;
    SET_DEFAULT(2);

    def = defs.add("ironing_type", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Ironing Type");
    def->category = L("Ironing");
    def->tooltip = L("Ironing Type");
    def->enum_type = IroningType::TopSurfaces;
    def->enum_values = { { int(IroningType::TopSurfaces), "top",     L("All top surfaces")     },
                         { int(IroningType::TopmostOnly), "topmost", L("Topmost surface only") },
                         { int(IroningType::AllSolid),    "solid",   L("All solid surfaces")   } };
    def->mode = comAdvanced;
    SET_DEFAULT(IroningType::TopSurfaces);

    def = defs.add("ironing_flowrate", Percent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Flow rate");
    def->category = L("Ironing");
    def->tooltip = L("Percent of a flow rate relative to object's normal layer height.");
    def->sidetext = L("%");
    def->ratio_over = "layer_height";
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(Percentage{15.});

    def = defs.add("ironing_spacing", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Spacing between ironing passes");
    def->category = L("Ironing");
    def->tooltip = L("Distance between ironing lines");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.1);

    def = defs.add("ironing_speed", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Ironing");
    def->category = L("Speed");
    def->tooltip = L("Ironing");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(15.);

    def = defs.add("layer_gcode", String);
    def->location = "printer_settings";
    def->label = L("After layer change G-code");
    def->tooltip = L("This custom code is inserted at every layer change, right after the Z move "
                   "and before the extruder moves to the first layer point. Note that you can use "
                   "placeholder variables for all Slic3r settings as well as [layer_num] and [layer_z].");
    def->cli = "after-layer-gcode|layer-gcode";
    def->multiline = true;
    def->full_width = true;
    def->height = 5;
    def->mode = comExpert;
    SET_DEFAULT("");

    def = defs.add("remaining_times", Bool);
    def->location = "printer_settings";
    def->label = L("Supports remaining times");
    def->tooltip = L("Emit M73 P[percent printed] R[remaining time in minutes] at 1 minute"
                     " intervals into the G-code to let the firmware show accurate remaining time."
                     " As of now only the Prusa i3 MK3 firmware recognizes M73."
                     " Also the i3 MK3 firmware supports M73 Qxx Sxx for the silent mode.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("silent_mode", Bool);
    def->location = "printer_settings";
    def->label = L("Supports stealth mode");
    def->tooltip = L("The firmware supports stealth mode");
    def->mode = comExpert;
    SET_DEFAULT(true);

    def = defs.add("binary_gcode", Bool);
    def->location = "printer_settings";
    def->label = L("Supports binary G-code");
    def->tooltip = L("Enable, if the firmware supports binary G-code format (bgcode). "
                     "To generate .bgcode files, make sure you have binary G-code enabled in Configuration->Preferences->Other.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("machine_limits_usage", Enum);
    def->location = "printer_settings";
    def->label = L("How to apply limits");
    def->full_label = L("Purpose of Machine Limits");
    def->category = L("Machine limits");
    def->tooltip = L("How to apply the Machine Limits");
    def->enum_type = MachineLimitsUsage::TimeEstimateOnly;
    def->enum_values = {
        { int(MachineLimitsUsage::EmitToGCode),      "emit_to_gcode",      L("Emit to G-code") },
        { int(MachineLimitsUsage::TimeEstimateOnly), "time_estimate_only", L("Use for time estimate") },
        { int(MachineLimitsUsage::Ignore),           "ignore",             L("Ignore") } };
    def->mode = comAdvanced;
    SET_DEFAULT(MachineLimitsUsage::TimeEstimateOnly);

    {
        struct AxisDefault {
            std::string         name;
            std::vector<double> max_feedrate;
            std::vector<double> max_acceleration;
            std::vector<double> max_jerk;
        };
        std::vector<AxisDefault> axes {
            // name, max_feedrate,  max_acceleration, max_jerk
            { "x", { 500., 200. }, {  9000., 1000. }, { 10. , 10.  } },
            { "y", { 500., 200. }, {  9000., 1000. }, { 10. , 10.  } },
            { "z", {  12.,  12. }, {   500.,  200. }, {  0.2,  0.4 } },
            { "e", { 120., 120. }, { 10000., 5000. }, {  2.5,  2.5 } }
        };
        for (const AxisDefault &axis : axes) {
            std::string axis_upper = boost::to_upper_copy<std::string>(axis.name);
            // Add the machine feedrate limits for XYZE axes. (M203)
            def = defs.add("machine_max_feedrate_" + axis.name, Doubles);
            def->location = "printer_settings";
            def->full_label = (boost::format("Maximum feedrate %1%") % axis_upper).str();
            (void)L("Maximum feedrate X");
            (void)L("Maximum feedrate Y");
            (void)L("Maximum feedrate Z");
            (void)L("Maximum feedrate E");
            def->category = L("Machine limits");
            def->tooltip  = (boost::format("Maximum feedrate of the %1% axis") % axis_upper).str();
            (void)L("Maximum feedrate of the X axis");
            (void)L("Maximum feedrate of the Y axis");
            (void)L("Maximum feedrate of the Z axis");
            (void)L("Maximum feedrate of the E axis");
            def->sidetext = L("mm/s");
            def->min = 0;
            def->mode = comAdvanced;
            const std::vector<double> max_feedrate = axis.max_feedrate;
            def->init_fn = [max_feedrate](ConfigItem& item) { item.vec<double>() = { max_feedrate }; };

            // Add the machine acceleration limits for XYZE axes (M201)
            def = defs.add("machine_max_acceleration_" + axis.name, Doubles);
            def->location = "printer_settings";
            def->full_label = (boost::format("Maximum acceleration %1%") % axis_upper).str();
            (void)L("Maximum acceleration X");
            (void)L("Maximum acceleration Y");
            (void)L("Maximum acceleration Z");
            (void)L("Maximum acceleration E");
            def->category = L("Machine limits");
            def->tooltip  = (boost::format("Maximum acceleration of the %1% axis") % axis_upper).str();
            (void)L("Maximum acceleration of the X axis");
            (void)L("Maximum acceleration of the Y axis");
            (void)L("Maximum acceleration of the Z axis");
            (void)L("Maximum acceleration of the E axis");
            def->sidetext = L("mm/s²");
            def->min = 0;
            def->mode = comAdvanced;
            const std::vector<double> max_acceleration = axis.max_acceleration;
            def->init_fn = [max_acceleration](ConfigItem& item) { item.vec<double>() = { max_acceleration }; };

            // Add the machine jerk limits for XYZE axes (M205)
            def = defs.add("machine_max_jerk_" + axis.name, Doubles);
            def->location = "printer_settings";
            def->full_label = (boost::format("Maximum jerk %1%") % axis_upper).str();
            (void)L("Maximum jerk X");
            (void)L("Maximum jerk Y");
            (void)L("Maximum jerk Z");
            (void)L("Maximum jerk E");
            def->category = L("Machine limits");
            def->tooltip  = (boost::format("Maximum jerk of the %1% axis") % axis_upper).str();
            (void)L("Maximum jerk of the X axis");
            (void)L("Maximum jerk of the Y axis");
            (void)L("Maximum jerk of the Z axis");
            (void)L("Maximum jerk of the E axis");
            def->sidetext = L("mm/s");
            def->min = 0;
            def->mode = comAdvanced;
            const std::vector<double> max_jerk = axis.max_jerk;
            def->init_fn = [max_jerk](ConfigItem& item) { item.vec<double>() = { max_jerk }; };
        }
    }
    
    // M205 S... [mm/sec]
    def = defs.add("machine_min_extruding_rate", Doubles);
    def->location = "printer_settings";
    def->full_label = L("Minimum feedrate when extruding");
    def->category = L("Machine limits");
    def->tooltip = L("Minimum feedrate when extruding (M205 S)");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comAdvanced;
    def->init_fn = [](ConfigItem& item) { item.vec<double>() = { 0., 0. }; };

    // M205 T... [mm/sec]
    def = defs.add("machine_min_travel_rate", Doubles);
    def->location = "printer_settings";
    def->full_label = L("Minimum travel feedrate");
    def->category = L("Machine limits");
    def->tooltip = L("Minimum travel feedrate (M205 T)");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comAdvanced;
    def->init_fn = [](ConfigItem& item) { item.vec<double>() = { 0., 0. }; };

    // M204 P... [mm/sec^2]
    def = defs.add("machine_max_acceleration_extruding", Doubles);
    def->location = "printer_settings";
    def->full_label = L("Maximum acceleration when extruding");
    def->category = L("Machine limits");
    def->tooltip = L("Maximum acceleration when extruding");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comAdvanced;
    def->init_fn = [](ConfigItem& item) { item.vec<double>() = { 1500., 1250. }; };

    // M204 R... [mm/sec^2]
    def = defs.add("machine_max_acceleration_retracting", Doubles);
    def->location = "printer_settings";
    def->full_label = L("Maximum acceleration when retracting");
    def->category = L("Machine limits");
    def->tooltip = L("Maximum acceleration when retracting.\n\n"
                     "Not used for RepRapFirmware, which does not support it.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comAdvanced;
    def->init_fn = [](ConfigItem& item) { item.vec<double>() = { 1500., 1250. }; };

    // M204 T... [mm/sec^2]
    def = defs.add("machine_max_acceleration_travel", Doubles);
    def->location = "printer_settings";
    def->full_label = L("Maximum acceleration for travel moves");
    def->category = L("Machine limits");
    def->tooltip = L("Maximum acceleration for travel moves.");
    def->sidetext = L("mm/s²");
    def->min = 0;
    def->mode = comAdvanced;
    def->init_fn = [](ConfigItem& item) { item.vec<double>() = { 1500., 1250. }; };

    def = defs.add("max_fan_speed", Int);
    def->location = "filament_settings";
    def->label = L("Max");
    def->tooltip = L("This setting represents the maximum speed of your fan.");
    def->sidetext = L("%");
    def->min = 0;
    def->max = 100;
    def->mode = comExpert;
    SET_DEFAULT(100);

    def = defs.add("max_layer_height", Double);
    def->location = "toolprint_settings";
    def->label = L("Max");
    def->tooltip = L("This is the highest printable layer height for this extruder, used to cap "
                   "the variable layer height and support layer height. Maximum recommended layer height "
                   "is 75% of the extrusion width to achieve reasonable inter-layer adhesion. "
                   "If set to 0, layer height is limited to 75% of the nozzle diameter.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("max_print_speed", Double);
    def->location = "print_settings";
    def->label = L("Max print speed");
    def->tooltip = L("When setting other speed settings to 0 Slic3r will autocalculate the optimal speed "
                   "in order to keep constant extruder pressure. This experimental setting is used "
                   "to set the highest print speed you want to allow.");
    def->sidetext = L("mm/s");
    def->min = 1;
    def->mode = comExpert;
    SET_DEFAULT(80.);

    def = defs.add("max_volumetric_speed", Double);
    def->location = "print_settings";
    def->label = L("Max volumetric speed");
    def->tooltip = L("This experimental setting is used to set the maximum volumetric speed your "
                   "extruder supports.");
    def->sidetext = L("mm³/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("max_volumetric_extrusion_rate_slope_positive", Double);
    def->location = "print_settings";
    def->label = L("Max volumetric slope positive");
    def->tooltip = L("This experimental setting is used to limit the speed of change in extrusion rate "
                       "for a transition from lower speed to higher speed. "
                   "A value of 1.8 mm³/s² ensures, that a change from the extrusion rate "
                   "of 1.8 mm³/s (0.45 mm extrusion width, 0.2 mm extrusion height, feedrate 20 mm/s) "
                   "to 5.4 mm³/s (feedrate 60 mm/s) will take at least 2 seconds.");
    def->sidetext = L("mm³/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("max_volumetric_extrusion_rate_slope_negative", Double);
    def->location = "print_settings";
    def->label = L("Max volumetric slope negative");
    def->tooltip = L("This experimental setting is used to limit the speed of change in extrusion rate "
                       "for a transition from higher speed to lower speed. "
                   "A value of 1.8 mm³/s² ensures, that a change from the extrusion rate "
                   "of 5.4 mm³/s (0.45 mm extrusion width, 0.2 mm extrusion height, feedrate 60 mm/s) "
                   "to 1.8 mm³/s (feedrate 20 mm/s) will take at least 2 seconds.");
    def->sidetext = L("mm³/s²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("min_fan_speed", Int);
    def->location = "filament_settings";
    def->label = L("Min");
    def->tooltip = L("This setting represents the minimum PWM your fan needs to work.");
    def->sidetext = L("%");
    def->min = 0;
    def->max = 100;
    def->mode = comExpert;
    SET_DEFAULT(35);

    def = defs.add("min_layer_height", Double);
    def->location = "toolprint_settings";
    def->label = L("Min");
    def->tooltip = L("This is the lowest printable layer height for this extruder and limits "
                   "the resolution for variable layer height. Typical values are between 0.05 mm and 0.1 mm.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.07);

    def = defs.add("min_print_speed", Double);
    def->location = "filament_settings";
    def->label = L("Min print speed");
    def->tooltip = L("Slic3r will not scale speed down below this speed.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(10.);

    def = defs.add("min_skirt_length", Double);
    def->location = "print_settings";
    def->label = L("Minimal filament extrusion length");
    def->tooltip = L("Generate no less than the number of skirt loops required to consume "
                   "the specified amount of filament on the bottom layer. For multi-extruder machines, "
                   "this minimum applies to each extruder.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("notes", String);
    def->location = "print_settings";
    def->label = L("Configuration notes");
    def->tooltip = L("You can put here your personal notes. This text will be added to the G-code "
                   "header comments.");
    def->multiline = true;
    def->full_width = true;
    def->height = 13;
    def->mode = comAdvanced;
    SET_DEFAULT("");

    def = defs.add("nozzle_diameter", Double);
    def->location = "toolprint_settings";
    def->label = L("Nozzle diameter");
    def->tooltip = L("This is the diameter of your extruder nozzle (for example: 0.5, 0.35 etc.)");
    def->sidetext = L("mm");
    SET_DEFAULT(0.4);

    def = defs.add("only_retract_when_crossing_perimeters", Bool);
    def->location = "print_settings";
    def->label = L("Only retract when crossing perimeters");
    def->tooltip = L("Disables retraction when the travel path does not exceed the upper layer's perimeters "
                   "(and thus any ooze will be probably invisible).");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("ooze_prevention", Bool);
    def->location = "print_settings";
    def->label = L("Enable");
    // TRN PrintSettings: Enable ooze prevention
    def->tooltip = L("This option will drop the temperature of the inactive extruders to prevent oozing.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("overhangs", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Detect bridging perimeters");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Experimental option to adjust flow for overhangs (bridge flow will be used), "
                   "to apply bridge speed to them and enable fan.");
    def->mode = comAdvanced;
    SET_DEFAULT(true);

    def = defs.add("parking_pos_retraction", Double);
    def->location = "printer_settings";
    def->label = L("Filament parking position");
    def->tooltip = L("Distance of the extruder tip from the position where the filament is parked "
                      "when unloaded. This should match the value in printer firmware.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(92.);

    def = defs.add("extra_loading_move", Double);
    def->location = "printer_settings";
    def->label = L("Extra loading distance");
    def->tooltip = L("When set to zero, the distance the filament is moved from parking position during load "
                      "is exactly the same as it was moved back during unload. When positive, it is loaded further, "
                      " if negative, the loading move is shorter than unloading.");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    SET_DEFAULT(-2.);

    def = defs.add("multimaterial_purging", Double);
    def->location = "printer_settings";
    def->label = L("Purging volume");
    def->tooltip = L("Determines purging volume on the wipe tower. This can be modified in Filament Settings "
                     "('filament_purge_multiplier') or overridden using project-specific settings.");
    def->sidetext = L("mm³");
    def->mode = comAdvanced;
    SET_DEFAULT(140.);

    def = defs.add("perimeter_acceleration", Double);
    def->location = "print_settings";
    def->label = L("Perimeters");
    def->tooltip = L("This is the acceleration your printer will use for perimeters. "
                     "Set zero to disable acceleration control for perimeters.");
    def->sidetext = L("mm/s²");
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("external_perimeter_acceleration", Double);
    def->location = "print_settings";
    def->label = L("External perimeters");
    def->tooltip = L("This is the acceleration your printer will use for external perimeters. "
                     "Set zero to use the value for perimeters.");
    def->sidetext = L("mm/s²");
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("perimeter_extruder", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Perimeter extruder");
    def->category = L("Extruders");
    def->tooltip = L("The extruder to use when printing perimeters and brim. First extruder is 1.");
    def->aliases = { "perimeters_extruder" };
    def->min = 1;
    def->mode = comAdvanced;
    SET_DEFAULT(1);

    def = defs.add("perimeter_extrusion_width", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Perimeters");
    def->category = L("Extrusion Width");
    def->tooltip = L("Set this to a non-zero value to set a manual extrusion width for perimeters. "
                   "You may want to use thinner extrudates to get more accurate surfaces. "
                   "If left zero, default extrusion width will be used if set, otherwise 1.125 x nozzle diameter will be used. "
                   "If expressed as percentage (for example 200%) it will be computed over layer height.");
    def->sidetext = L("mm or %");
    def->aliases = { "perimeters_extrusion_width" };
    def->min = 0;
    def->max_literal = 50;
    def->mode = comAdvanced;
    def->ratio_over = "layer_height";
    SET_DEFAULT(FloatOrPercentage{0.});

    def = defs.add("perimeter_speed", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Perimeters");
    def->category = L("Speed");
    def->tooltip = L("Speed for perimeters (contours, aka vertical shells). Set to zero for auto.");
    def->sidetext = L("mm/s");
    def->aliases = { "perimeter_feed_rate" };
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(60.);

    def = defs.add("perimeters", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Perimeters");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("This option sets the number of perimeters to generate for each layer. "
                   "Note that Slic3r may increase this number automatically when it detects "
                   "sloping surfaces which benefit from a higher number of perimeters "
                   "if the Extra Perimeters option is enabled.");
    def->sidetext = L("(minimum)");
    def->aliases = { "perimeter_offsets" };
    def->min = 0;
    def->max = 10000;
    SET_DEFAULT(3);

    def = defs.add("post_process", Strings);
    def->location = "print_settings";
    def->label = L("Post-processing scripts");
    def->tooltip = L("If you want to process the output G-code through custom scripts, "
                   "just list their absolute paths here. Separate multiple scripts with a semicolon. "
                   "Scripts will be passed the absolute path to the G-code file as the first argument, "
                   "and they can access the Slic3r config settings by reading environment variables.");
    def->gui_flags = "serialized";
    def->multiline = true;
    def->full_width = true;
    def->height = 6;
    def->mode = comExpert;
    def->init_fn = [](ConfigItem& item) { item.vec<std::string>() = {}; };

    def = defs.add("printer_model", String);
    def->location = "printer_settings";
    def->label = L("Printer type");
    def->tooltip = L("Type of the printer.");
    SET_DEFAULT("");

    def = defs.add("printer_notes", String);
    def->location = "printer_settings";
    def->label = L("Printer notes");
    def->tooltip = L("You can put your notes regarding the printer here.");
    def->multiline = true;
    def->full_width = true;
    def->height = 13;
    def->mode = comAdvanced;
    SET_DEFAULT("");

    def = defs.add("printer_vendor", String);
    def->location = "printer_settings";
    def->label = L("Printer vendor");
    def->tooltip = L("Name of the printer vendor.");
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT("");
    

    def = defs.add("printer_variant", String);
    def->location = "printer_settings";
    def->label = L("Printer variant");
    def->tooltip = L("Name of the printer variant. For example, the printer variants may be differentiated by a nozzle diameter.");
    SET_DEFAULT("");

    /* TODO: What about this?
    def = defs.add("print_settings_id", String);
    SET_DEFAULT(""));
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("printer_settings_id", String);
    SET_DEFAULT(""));
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("physical_printer_settings_id", String);
    SET_DEFAULT(""));
    def->cli = ConfigOptionDef::nocli;*/

    def = defs.add("raft_contact_distance", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Raft contact Z distance");
    def->category = L("Support material");
    def->tooltip = L("The vertical distance between object and raft. Ignored for soluble interface.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.1);

    def = defs.add("raft_expansion", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Raft expansion");
    def->category = L("Support material");
    def->tooltip = L("Expansion of the raft in XY plane for better stability.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(1.5);

    def = defs.add("raft_first_layer_density", Percent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("First layer density");
    def->category = L("Support material");
    def->tooltip = L("Density of the first raft or support layer.");
    def->sidetext = L("%");
    def->min = 10;
    def->max = 100;
    def->mode = comExpert;
    SET_DEFAULT(Percentage{90.});

    def = defs.add("raft_first_layer_expansion", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("First layer expansion");
    def->category = L("Support material");
    def->tooltip = L("Expansion of the first raft or support layer to improve adhesion to print bed.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(3.);

    def = defs.add("raft_layers", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Raft layers");
    def->category = L("Support material");
    def->tooltip = L("The object will be raised by this number of layers, and support material "
                   "will be generated under it.");
    def->sidetext = L("layers");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0);

    def = defs.add("resolution", Double);
    def->location = "print_settings";
    def->label = L("Slice resolution");
    def->tooltip = L("Minimum detail resolution, used to simplify the input file for speeding up "
                   "the slicing job and reducing memory usage. High-resolution models often carry "
                   "more detail than printers can render. Set to zero to disable any simplification "
                   "and use full resolution from input.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("gcode_resolution", Double);
    def->location = "print_settings";
    def->label = L("G-code resolution");
    def->tooltip = L("Maximum deviation of exported G-code paths from their full resolution counterparts. "
                     "Very high resolution G-code requires huge amount of RAM to slice and preview, "
                     "also a 3D printer may stutter not being able to process a high resolution G-code in a timely manner. "
                     "On the other hand, a low resolution G-code will produce a low poly effect and because "
                     "the G-code reduction is performed at each layer independently, visible artifacts may be produced.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.0125);

    def = defs.add("retract_before_travel", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Minimum travel after retraction");
    def->tooltip = L("Retraction is not triggered when travel moves are shorter than this length.");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    SET_DEFAULT(2.);

    def = defs.add("retract_before_wipe", Percent);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Retract amount before wipe");
    def->tooltip = L("With bowden extruders, it may be wise to do some amount of quick retract "
                   "before doing the wipe movement.");
    def->sidetext = L("%");
    def->mode = comAdvanced;
    SET_DEFAULT(Percentage{0.});

    def = defs.add("retract_layer_change", Bool);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Retract on layer change");
    def->tooltip = L("This flag enforces a retraction whenever a Z move is done.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("retract_length", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Retraction length");
    def->full_label = L("Retraction Length");
    def->tooltip = L("When retraction is triggered, filament is pulled back by the specified amount "
                   "(the length is measured on raw filament, before it enters the extruder).");
    def->sidetext = L("mm (zero to disable)");
    SET_DEFAULT(2.);

    def = defs.add("retract_length_toolchange", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Length");
    def->full_label = L("Retraction Length (Toolchange)");
    def->tooltip = L("When retraction is triggered before changing tool, filament is pulled back "
                   "by the specified amount (the length is measured on raw filament, before it enters "
                   "the extruder).");
    def->sidetext = L("mm (zero to disable)");
    def->mode = comExpert;
    SET_DEFAULT(10.);

    def = defs.add("travel_slope", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Ramping slope angle");
    def->tooltip = L("Slope of the ramp in the initial phase of the travel.");
    def->sidetext = L("°");
    def->min = 0;
    def->max = 90;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("travel_ramping_lift", Bool);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Use ramping lift");
    def->tooltip = L("Generates a ramping lift instead of lifting the extruder directly upwards. "
                     "The travel is split into two phases: the ramp and the standard horizontal travel. "
                     "This option helps reduce stringing.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("travel_max_lift", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Maximum ramping lift");
    def->tooltip = L("Maximum lift height of the ramping lift. It may not be reached if the next position "
                     "is close to the old one.");
    def->sidetext = L("mm");
    def->min = 0;
    def->max_literal = 1000;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("travel_lift_before_obstacle", Bool);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Steeper ramp before obstacles");
    def->tooltip = L("If enabled, PrusaSlicer detects obstacles along the travel path and makes the slope steeper "
                     "in case an obstacle might be hit during the initial phase of the travel.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("nozzle_high_flow", Bool);
    def->location = "toolprint_settings";
    def->label = L("High flow nozzle");
    def->tooltip = L("High flow nozzles allow higher print speeds.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("retract_lift", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Lift height");
    def->tooltip = L("Lift height applied before travel.");
    def->sidetext = L("mm");
    def->min = 0;
    def->max_literal = 1000;
    def->mode = comSimple;
    SET_DEFAULT(0.);

    def = defs.add("retract_lift_above", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Above Z");
    def->full_label = L("Only lift Z above");
    def->tooltip = L("If you set this to a positive value, Z lift will only take place above the specified "
                   "absolute Z. You can tune this setting for skipping lift on the first layers.");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("retract_lift_below", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Below Z");
    def->full_label = L("Only lift Z below");
    def->tooltip = L("If you set this to a positive value, Z lift will only take place below "
                   "the specified absolute Z. You can tune this setting for limiting lift "
                   "to the first layers.");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("retract_restart_extra", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Deretraction extra length");
    def->tooltip = L("When the retraction is compensated after the travel move, the extruder will push "
                   "this additional amount of filament. This setting is rarely needed.");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("retract_restart_extra_toolchange", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Extra length on restart");
    def->tooltip = L("When the retraction is compensated after changing tool, the extruder will push "
                   "this additional amount of filament.");
    def->sidetext = L("mm");
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("retract_speed", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Retraction Speed");
    def->full_label = L("Retraction Speed");
    def->tooltip = L("The speed for retractions (it only applies to the extruder motor).");
    def->sidetext = L("mm/s");
    def->mode = comAdvanced;
    SET_DEFAULT(40.);

    def = defs.add("deretract_speed", Double);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Deretraction Speed");
    def->full_label = L("Deretraction Speed");
    def->tooltip = L("The speed for loading of a filament into extruder after retraction "
                   "(it only applies to the extruder motor). If left to zero, the retraction speed is used.");
    def->sidetext = L("mm/s");
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("seam_gap_distance", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Seam gap distance");
    def->tooltip = L("The distance between the endpoints of a closed loop perimeter. "
                   "Positive values will shorten and interrupt the loop slightly to reduce the seam. "
                   "Negative values will extend the loop, causing the endpoints to overlap slightly. "
                   "When percents are used, the distance is derived from the nozzle diameter. "
                   "Set to zero to disable this feature.");
    def->sidetext = L("mm or %");
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage(Percentage{15.}))

    def = defs.add("seam_position", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Seam position");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Position of perimeters starting points.");
    def->enum_type = SeamPosition::spRandom;
    def->enum_values = { { int(SeamPosition::spRandom),  "random",  L("Random") },
                         { int(SeamPosition::spNearest), "nearest", L("Nearest") },
                         { int(SeamPosition::spAligned), "aligned", L("Aligned") },
                         { int(SeamPosition::spRear),    "rear",    L("Rear") } };
    def->mode = comSimple;
    SET_DEFAULT(SeamPosition::spAligned);

    def = defs.add("staggered_inner_seams", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Staggered inner seams");
    // TRN PrintSettings: "Staggered inner seams"
    def->tooltip = L("This option causes the inner seams to be shifted backwards based on their depth, forming a zigzag pattern.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("scarf_seam_placement", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Scarf joint placement");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Where to place scarf joint seam.");
    def->mode = comAdvanced;
    def->enum_type = ScarfSeamPlacement::nowhere;
    def->enum_values = {
        // TRN: Drop-down option for 'Scarf joint placement' parameter.
        { int(ScarfSeamPlacement::nowhere),    "nowhere",    L("Nowhere") },
        // TRN: Drop-down option for 'Scarf joint placement' parameter.
        { int(ScarfSeamPlacement::countours),  "contours",   L("Contours") },
        { int(ScarfSeamPlacement::everywhere), "everywhere", L("Everywhere") } };
    SET_DEFAULT(ScarfSeamPlacement::nowhere);

    def = defs.add("scarf_seam_only_on_smooth", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Scarf joint only on smooth perimeters");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Only use the scarf joint when the perimeter is smooth.");
    def->mode = comAdvanced;
    SET_DEFAULT(true);

    def = defs.add("scarf_seam_start_height", Percent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Scarf start height");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Start height of the scarf joint specified as fraction of the current layer height.");
    def->sidetext = L("%");
    def->min = 0;
    def->max = 100;
    def->mode = comAdvanced;
    SET_DEFAULT(Percentage{0.});

    def = defs.add("scarf_seam_entire_loop", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Scarf joint around entire perimeter");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Extend the scarf around entire length of the perimeter.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("scarf_seam_length", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Scarf joint length");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Length of the scarf joint.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(20.);

    def = defs.add("scarf_seam_max_segment_length", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Max scarf joint segment length");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Maximum length of any scarf joint segment.");
    def->sidetext = L("mm");
    def->min = 0.15f;
    def->mode = comAdvanced;
    SET_DEFAULT(1.0);

    def = defs.add("scarf_seam_on_inner_perimeters", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Scarf joint on inner perimeters");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Use scarf joint on inner perimeters.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("skirt_distance", Double);
    def->location = "print_settings";
    def->label = L("Distance from brim/object");
    def->tooltip = L("Distance between skirt and brim (when draft shield is not used) or objects.");
    def->sidetext = L("mm");
    def->min = 0;
    SET_DEFAULT(6.);

    def = defs.add("skirt_height", Int);
    def->location = "print_settings";
    def->label = L("Skirt height");
    def->tooltip = L("Height of skirt expressed in layers.");
    def->sidetext = L("layers");
    def->mode = comAdvanced;
    SET_DEFAULT(1);

    def = defs.add("draft_shield", Enum);
    def->location = "print_settings";
    def->label = L("Draft shield");
    def->tooltip = L("With draft shield active, the skirt will be printed skirt_distance from the object, possibly intersecting brim.\n"
                     "Enabled = skirt is as tall as the highest printed object.\n"
                     "Limited = skirt is as tall as specified by skirt_height.\n"
    				 "This is useful to protect an ABS or ASA print from warping and detaching from print bed due to wind draft.");
    def->enum_type = DraftShield::dsDisabled;
    def->enum_values = { { int(DraftShield::dsDisabled), "disabled", L("Disabled") },
                         { int(DraftShield::dsLimited),  "limited",  L("Limited")},
                         { int(DraftShield::dsEnabled),  "enabled",  L("Enabled") } };
    def->mode = comAdvanced;
    SET_DEFAULT(DraftShield::dsDisabled);

    def = defs.add("skirts", Int);
    def->location = "print_settings";
    def->label = L("Loops (minimum)");
    def->full_label = L("Skirt Loops");
    def->tooltip = L("Number of loops for the skirt. If the Minimum Extrusion Length option is set, "
                   "the number of loops might be greater than the one configured here. Set this to zero "
                   "to disable skirt completely.");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(1);

    def = defs.add("slowdown_below_layer_time", Int);
    def->location = "filament_settings";
    def->label = L("Slow down if layer print time is below");
    def->tooltip = L("If layer print time is estimated below this number of seconds, print moves "
                   "speed will be scaled down to extend duration to this value.");
    def->sidetext = L("approximate seconds");
    def->min = 0;
    def->max = 1000;
    def->mode = comExpert;
    SET_DEFAULT(5);

    def = defs.add("small_perimeter_speed", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Small perimeters");
    def->category = L("Speed");
    def->tooltip = L("This separate setting will affect the speed of perimeters having radius <= 6.5mm "
                   "(usually holes). If expressed as percentage (for example: 80%) it will be calculated "
                   "on the perimeters speed setting above. Set to zero for auto.");
    def->sidetext = L("mm/s or %");
    def->ratio_over = "perimeter_speed";
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage{15.});

    def = defs.add("solid_infill_below_area", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Solid infill threshold area");
    def->category = L("Infill");
    def->tooltip = L("Force solid infill for regions having a smaller area than the specified threshold.");
    def->sidetext = L("mm²");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(70.);

    def = defs.add("solid_infill_extruder", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Solid infill extruder");
    def->category = L("Extruders");
    def->tooltip = L("The extruder to use when printing solid infill.");
    def->min = 1;
    def->mode = comAdvanced;
    SET_DEFAULT(1);

    def = defs.add("solid_infill_every_layers", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Solid infill every");
    def->category = L("Infill");
    def->tooltip = L("This feature allows to force a solid layer every given number of layers. "
                   "Zero to disable. You can set this to any value (for example 9999); "
                   "Slic3r will automatically choose the maximum possible number of layers "
                   "to combine according to nozzle diameter and layer height.");
    def->sidetext = L("layers");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0);

    def = defs.add("solid_infill_extrusion_width", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Solid infill");
    def->category = L("Extrusion Width");
    def->tooltip = L("Set this to a non-zero value to set a manual extrusion width for infill for solid surfaces. "
                   "If left zero, default extrusion width will be used if set, otherwise 1.125 x nozzle diameter will be used. "
                   "If expressed as percentage (for example 90%) it will be computed over layer height.");
    def->sidetext = L("mm or %");
    def->min = 0;
    def->max_literal = 50;
    def->mode = comAdvanced;
    def->ratio_over = "layer_height";
    SET_DEFAULT(FloatOrPercentage{0.});

    def = defs.add("solid_infill_speed", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Solid infill");
    def->category = L("Speed");
    def->tooltip = L("Speed for printing solid regions (top/bottom/internal horizontal shells). "
                   "This can be expressed as a percentage (for example: 80%) over the default "
                   "infill speed above. Set to zero for auto.");
    def->sidetext = L("mm/s or %");
    def->ratio_over = "infill_speed";
    def->aliases = { "solid_infill_feed_rate" };
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage{20.});

    /* TODO : shortcut - maybe remove it?
    def = defs.add("solid_layers", Int);
    def->label = L("Solid layers");
    def->tooltip = L("Number of solid layers to generate on top and bottom surfaces.");
    def->shortcut.push_back("top_solid_layers");
    def->shortcut.push_back("bottom_solid_layers");
    def->min = 0;

    def = defs.add("solid_min_thickness", Double);
    def->label = L("Minimum thickness of a top / bottom shell");
    def->tooltip = L("Minimum thickness of a top / bottom shell");
    def->shortcut.push_back("top_solid_min_thickness");
    def->shortcut.push_back("bottom_solid_min_thickness");
    def->min = 0;*/

    def = defs.add("spiral_vase", Bool);
    def->location = "print_settings";
    def->label = L("Spiral vase");
    def->tooltip = L("This feature will raise Z gradually while printing a single-walled object "
                   "in order to remove any visible seam. This option requires a single perimeter, "
                   "no infill, no top solid layers and no support material. You can still set "
                   "any number of bottom solid layers as well as skirt/brim loops. "
                   "It won't work when printing more than one single object.");
    SET_DEFAULT(false);

    def = defs.add("standby_temperature_delta", Int);
    def->location = "print_settings";
    def->label = L("Temperature variation");
    // TRN PrintSettings : "Ooze prevention" > "Temperature variation"
    def->tooltip = L("Temperature difference to be applied when an extruder is not active. "
                     "The value is not used when 'idle_temperature' in filament settings "
                     "is defined.");
    def->sidetext = "∆°C";
    def->min = -max_temp;
    def->max = max_temp;
    def->mode = comExpert;
    SET_DEFAULT(-5);

    def = defs.add("autoemit_temperature_commands", Bool);
    def->location = "printer_settings";
    def->label = L("Emit temperature commands automatically");
    def->tooltip = L("When enabled, PrusaSlicer will check whether your custom Start G-Code contains G-codes to set "
                     "extruder, bed or chamber temperature (M104, M109, M140, M190, M141 and M191). "
                     "If so, the temperatures will not be emitted automatically so you're free to customize "
                     "the order of heating commands and other custom actions. Note that you can use "
                     "placeholder variables for all PrusaSlicer settings, so you can put "
                     "a \"M109 S[first_layer_temperature]\" command wherever you want.\n"
                     "If your custom Start G-Code does NOT contain these G-codes, "
                     "PrusaSlicer will execute the Start G-Code after heated chamber was set to its temperature, "
                     "bed reached its target temperature and extruder just started heating.\n\n"
                     "When disabled, PrusaSlicer will NOT emit commands to heat up extruder, bed or chamber, "
                     "leaving all to Custom Start G-Code.");
    def->mode = comExpert;
    SET_DEFAULT(true);

    def = defs.add("start_gcode", String);
    def->location = "printer_settings";
    def->label = L("Start G-code");
    def->tooltip = L("This start procedure is inserted at the beginning, possibly prepended by "
                     "temperature-changing commands. See 'autoemit_temperature_commands'.");
    def->multiline = true;
    def->full_width = true;
    def->height = 12;
    def->mode = comExpert;
    SET_DEFAULT("G28 ; home all axes\nG1 Z5 F5000 ; lift nozzle\n");

    def = defs.add("start_filament_gcode", String);
    def->location = "filament_settings";
    def->label = L("Start G-code");
    def->tooltip = L("This start procedure is inserted at the beginning, after any printer start gcode (and "
                   "after any toolchange to this filament in case of multi-material printers). "
                   "This is used to override settings for a specific filament. If PrusaSlicer detects "
                   "M104, M109, M140 or M190 in your custom codes, such commands will "
                   "not be prepended automatically so you're free to customize the order "
                   "of heating commands and other custom actions. Note that you can use placeholder variables "
                   "for all PrusaSlicer settings, so you can put a \"M109 S[first_layer_temperature]\" command "
                   "wherever you want. If you have multiple extruders, the gcode is processed "
                   "in extruder order.");
    def->multiline = true;
    def->full_width = true;
    def->height = 12;
    def->mode = comExpert;
    SET_DEFAULT("; Filament gcode\n");

    def = defs.add("color_change_gcode", String);
    def->location = "printer_settings";
    def->label = L("Color change G-code");
    def->tooltip = L("This G-code will be used as a code for the color change");
    def->multiline = true;
    def->full_width = true;
    def->height = 12;
    def->mode = comExpert;
    SET_DEFAULT("M600");

    def = defs.add("pause_print_gcode", String);
    def->location = "printer_settings";
    def->label = L("Pause Print G-code");
    def->tooltip = L("This G-code will be used as a code for the pause print");
    def->multiline = true;
    def->full_width = true;
    def->height = 12;
    def->mode = comExpert;
    SET_DEFAULT("M601");

    def = defs.add("template_custom_gcode", String);
    def->location = "printer_settings";
    def->label = L("Custom G-code");
    def->tooltip = L("This G-code will be used as a custom code");
    def->multiline = true;
    def->full_width = true;
    def->height = 12;
    def->mode = comExpert;
    SET_DEFAULT("");

    def = defs.add("single_extruder_multi_material", Bool);
    def->location = "printer_settings";
    def->label = L("Single Extruder Multi Material");
    def->tooltip = L("The printer multiplexes filaments into a single hot end.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("single_extruder_multi_material_priming", Bool);
    def->location = "printer_settings";
    def->label = L("Prime all printing extruders");
    def->tooltip = L("If enabled, all printing extruders will be primed at the front edge of the print bed at the start of the print.");
    def->mode = comAdvanced;
    SET_DEFAULT(true);

    def = defs.add("wipe_tower_no_sparse_layers", Bool);
    def->location = "print_settings";
    def->label = L("No sparse layers (EXPERIMENTAL)");
    def->tooltip = L("If enabled, the wipe tower will not be printed on layers with no toolchanges. "
                     "On layers with a toolchange, extruder will travel downward to print the wipe tower. "
                     "User is responsible for ensuring there is no collision with the print.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("support_material", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Generate support material");
    def->category = L("Support material");
    def->tooltip = L("Enable support material generation.");
    SET_DEFAULT(false);

    def = defs.add("support_material_auto", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Auto generated supports");
    def->category = L("Support material");
    def->tooltip = L("If checked, supports will be generated automatically based on the overhang threshold value."\
                     " If unchecked, supports will be generated inside the \"Support Enforcer\" volumes only.");
    def->mode = comSimple;
    SET_DEFAULT(true);

    def = defs.add("support_material_xy_spacing", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("XY separation between an object and its support");
    def->category = L("Support material");
    def->tooltip = L("XY separation between an object and its support. If expressed as percentage "
                   "(for example 50%), it will be calculated over external perimeter width.");
    def->sidetext = L("mm or %");
    def->ratio_over = "external_perimeter_extrusion_width";
    def->min = 0;
    def->max_literal = 10;
    def->mode = comAdvanced;
    // Default is half the external perimeter width.
    SET_DEFAULT(FloatOrPercentage(Percentage{50.}));

    def = defs.add("support_material_angle", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Pattern angle");
    def->category = L("Support material");
    def->tooltip = L("Use this setting to rotate the support material pattern on the horizontal plane.");
    def->sidetext = L("°");
    def->min = 0;
    def->max = 359;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("support_material_buildplate_only", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Support on build plate only");
    def->category = L("Support material");
    def->tooltip = L("Only create support if it lies on a build plate. Don't create support on a print.");
    def->mode = comSimple;
    SET_DEFAULT(false);

    def = defs.add("support_material_contact_distance", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Top contact Z distance");
    def->category = L("Support material");
    def->tooltip = L("The vertical distance between object and support material interface. "
                   "Setting this to 0 will also prevent Slic3r from using bridge flow and speed "
                   "for the first object layer.");
    def->sidetext = L("mm");
    def->choices = {
        { 0.,     L("0 (soluble)") },
        { 0.1,    L("0.1 (detachable)") },
        { 0.2,    L("0.2 (detachable)") }
    };
    def->mode = comAdvanced;
    SET_DEFAULT(0.2);

    def = defs.add("support_material_bottom_contact_distance", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Bottom contact Z distance");
    def->category = L("Support material");
    def->tooltip = L("The vertical distance between the object top surface and the support material interface. "
                   "If set to zero, support_material_contact_distance will be used for both top and bottom contact Z distances.");
    def->sidetext = L("mm");
    def->choices = {
        //TRN Print Settings: "Bottom contact Z distance". Have to be as short as possible
        { 0.,     L("Same as top") },
        { 0.1,    "0.1" },
        { 0.2,    "0.2" }
    };
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("support_material_enforce_layers", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Enforce support for the first");
    def->category = L("Support material");
    def->tooltip = L("Generate support material for the specified number of layers counting from bottom, "
                   "regardless of whether normal support material is enabled or not and regardless "
                   "of any angle threshold. This is useful for getting more adhesion of objects "
                   "having a very thin or poor footprint on the build plate.");
    def->sidetext = L("layers");
    def->full_label = L("Enforce support for the first n layers");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0);

    def = defs.add("support_material_extruder", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Support material/raft/skirt extruder");
    def->category = L("Extruders");
    def->tooltip = L("The extruder to use when printing support material, raft and skirt "
                   "(1+, 0 to use the current extruder to minimize tool changes).");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(1);

    def = defs.add("support_material_extrusion_width", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Support material");
    def->category = L("Extrusion Width");
    def->tooltip = L("Set this to a non-zero value to set a manual extrusion width for support material. "
                   "If left zero, default extrusion width will be used if set, otherwise nozzle diameter will be used. "
                   "If expressed as percentage (for example 90%) it will be computed over layer height.");
    def->sidetext = L("mm or %");
    def->min = 0;
    def->max_literal = 50;
    def->mode = comAdvanced;
    def->ratio_over = "layer_height";
    SET_DEFAULT(FloatOrPercentage{0.});

    def = defs.add("support_material_interface_contact_loops", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Interface loops");
    def->category = L("Support material");
    def->tooltip = L("Cover the top contact layer of the supports with loops. Disabled by default.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("support_material_interface_extruder", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Support material/raft interface extruder");
    def->category = L("Extruders");
    def->tooltip = L("The extruder to use when printing support material interface "
                   "(1+, 0 to use the current extruder to minimize tool changes). This affects raft too.");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(1);

    def = defs.add("support_material_interface_layers", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Top interface layers");
    def->category = L("Support material");
    def->tooltip = L("Number of interface layers to insert between the object(s) and support material.");
    def->sidetext = L("layers");
    def->min = 0;
    def->choices = {
        { 0, L("0 (off)") },
        { 1, L("1 (light)") },
        { 2, L("2 (default)") },
        { 3, L("3 (heavy)") }
    };
    def->mode = comAdvanced;
    SET_DEFAULT(3);

    def = defs.add("support_material_bottom_interface_layers", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Bottom interface layers");
    def->category = L("Support material");
    def->tooltip = L("Number of interface layers to insert between the object(s) and support material. "
                     "Set to -1 to use support_material_interface_layers");
    def->sidetext = L("layers");
    def->min = -1;
    def->choices = {
        //TRN Print Settings: "Bottom interface layers". Have to be as short as possible
        { -1, L("Same as top") },
        { 0,  L("0 (off)") },
        { 1,  L("1 (light)") },
        { 2,  L("2 (default)") },
        { 3,  L("3 (heavy)") }
    };
    def->mode = comAdvanced;
    SET_DEFAULT(-1); 

    def = defs.add("support_material_closing_radius", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Closing radius");
    def->category = L("Support material");
    def->tooltip = L("For snug supports, the support regions will be merged using morphological closing operation."
                     " Gaps smaller than the closing radius will be filled in.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(2.);

    def = defs.add("support_material_interface_spacing", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Interface pattern spacing");
    def->category = L("Support material");
    def->tooltip = L("Spacing between interface lines. Set zero to get a solid interface.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("support_material_interface_speed", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Support material interface");
    def->category = L("Support material");
    def->tooltip = L("Speed for printing support material interface layers. If expressed as percentage "
                   "(for example 50%) it will be calculated over support material speed.");
    def->sidetext = L("mm/s or %");
    def->ratio_over = "support_material_speed";
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage(Percentage{100.}));

    def = defs.add("support_material_pattern", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Pattern");
    def->category = L("Support material");
    def->tooltip = L("Pattern used to generate support material.");
    def->enum_type = SupportMaterialPattern::smpRectilinear;
    def->enum_values = {
        { int(SupportMaterialPattern::smpRectilinear),      "rectilinear",      L("Rectilinear") },
        { int(SupportMaterialPattern::smpRectilinearGrid),  "rectilinear-grid", L("Rectilinear grid") },
        { int(SupportMaterialPattern::smpHoneycomb),        "honeycomb",         L("Honeycomb") } };
    def->mode = comAdvanced;
    SET_DEFAULT(SupportMaterialPattern::smpRectilinear);

    def = defs.add("support_material_interface_pattern", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Interface pattern");
    def->category = L("Support material");
    def->tooltip = L("Pattern used to generate support material interface. "
                     "Default pattern for non-soluble support interface is Rectilinear, "
                     "while default pattern for soluble support interface is Concentric.");
        def->enum_type = SupportMaterialInterfacePattern::smipAuto;
    def->enum_values = {
        { int(SupportMaterialInterfacePattern::smipAuto),        "auto",        L("Default") },
        { int(SupportMaterialInterfacePattern::smipRectilinear), "rectilinear", L("Rectilinear") },
        { int(SupportMaterialInterfacePattern::smipConcentric),  "concentric",  L("Concentric") } };
    def->mode = comAdvanced;
    SET_DEFAULT(SupportMaterialInterfacePattern::smipRectilinear);

    def = defs.add("support_material_spacing", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Pattern spacing");
    def->category = L("Support material");
    def->tooltip = L("Spacing between support material lines.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(2.5);

    def = defs.add("support_material_speed", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Support material");
    def->category = L("Support material");
    def->tooltip = L("Speed for printing support material.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(60.);

    def = defs.add("support_material_style", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Style");
    def->category = L("Support material");
    def->tooltip = L("Style and shape of the support towers. Projecting the supports into a regular grid "
                     "will create more stable supports, while snug support towers will save material and reduce "
                     "object scarring.");
    def->enum_type = SupportMaterialStyle::smsGrid;
    def->enum_values = { { int(SupportMaterialStyle::smsGrid),    "grid",    L("Grid") },
                         { int(SupportMaterialStyle::smsSnug),    "snug",    L("Snug") },
                         { int(SupportMaterialStyle::smsOrganic), "organic", L("Organic") } };
    def->mode = comAdvanced;
    SET_DEFAULT(SupportMaterialStyle::smsGrid);

    def = defs.add("support_material_synchronize_layers", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Synchronize with object layers");
    def->category = L("Support material");
    // TRN PrintSettings : "Synchronize with object layers"
    def->tooltip = L("Synchronize support layers with the object print layers. This is useful "
                   "with multi-material printers, where the extruder switch is expensive. "
                   "This option is only available when top contact Z distance is set to zero.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("support_material_threshold", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Overhang threshold");
    def->category = L("Support material");
    def->tooltip = L("Support material will not be generated for overhangs whose slope angle "
                   "(90° = vertical) is above the given threshold. In other words, this value "
                   "represent the most horizontal slope (measured from the horizontal plane) "
                   "that you can print without support material. Set to zero for automatic detection "
                   "(recommended).");
    def->sidetext = L("°");
    def->min = 0;
    def->max = 90;
    def->mode = comAdvanced;
    SET_DEFAULT(0);

    def = defs.add("support_material_with_sheath", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("With sheath around the support");
    def->category = L("Support material");
    def->tooltip = L("Add a sheath (a single perimeter line) around the base support. This makes "
                   "the support more reliable, but also more difficult to remove.");
    def->mode = comExpert;
    SET_DEFAULT(true);

    def = defs.add("support_tree_angle", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Maximum Branch Angle");
    def->category = L("Support material");
    // TRN PrintSettings: "Organic supports" > "Maximum Branch Angle"
    def->tooltip = L("The maximum angle of the branches, when the branches have to avoid the model. "
                     "Use a lower angle to make them more vertical and more stable. Use a higher angle to be able to have more reach.");
    def->sidetext = L("°");
    def->min = 0;
    def->max = 85;
    def->mode = comAdvanced;
    SET_DEFAULT(40.);

    def = defs.add("support_tree_angle_slow", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Preferred Branch Angle");
    def->category = L("Support material");
    // TRN PrintSettings: "Organic supports" > "Preferred Branch Angle"
    def->tooltip = L("The preferred angle of the branches, when they do not have to avoid the model. "
                     "Use a lower angle to make them more vertical and more stable. Use a higher angle for branches to merge faster.");
    def->sidetext = L("°");
    def->min = 10;
    def->max = 85;
    def->mode = comAdvanced;
    SET_DEFAULT(25.);

    def = defs.add("support_tree_tip_diameter", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Tip Diameter");
    def->category = L("Support material");
    // TRN PrintSettings: "Organic supports" > "Tip Diameter"
    def->tooltip = L("Branch tip diameter for organic supports.");
    def->sidetext = L("mm");
    def->min = 0.1f;
    def->max = 100.f;
    def->mode = comAdvanced;
    SET_DEFAULT(0.8);

    def = defs.add("support_tree_branch_diameter", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Branch Diameter");
    def->category = L("Support material");
    // TRN PrintSettings: "Organic supports" > "Branch Diameter"
    def->tooltip = L("The diameter of the thinnest branches of organic support. Thicker branches are more sturdy. "
                     "Branches towards the base will be thicker than this.");
    def->sidetext = L("mm");
    def->min = 0.1f;
    def->max = 100.f;
    def->mode = comAdvanced;
    SET_DEFAULT(2.);

    def = defs.add("support_tree_branch_diameter_angle", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Branch Diameter Angle");
    def->category = L("Support material");
    // TRN PrintSettings: "Organic supports" > "Branch Diameter Angle"
    def->tooltip = L("The angle of the branches' diameter as they gradually become thicker towards the bottom. "
                     "An angle of 0 will cause the branches to have uniform thickness over their length. "
                     "A bit of an angle can increase stability of the organic support.");
    def->sidetext = L("°");
    def->min = 0;
    def->max = 15;
    def->mode = comAdvanced;
    SET_DEFAULT(5.);

    def = defs.add("support_tree_branch_diameter_double_wall", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Branch Diameter with double walls");
    def->category = L("Support material");
    // TRN PrintSettings: "Organic supports" > "Branch Diameter"
    def->tooltip = L("Branches with area larger than the area of a circle of this diameter will be printed with double walls for stability. "
                     "Set this value to zero for no double walls.");
    def->sidetext = L("mm");
    def->min = 0;
    def->max = 100.f;
    def->mode = comAdvanced;
    SET_DEFAULT(3.);

    // Tree Support Branch Distance
    // How far apart the branches need to be when they touch the model. Making this distance small will cause 
    // the tree support to touch the model at more points, causing better overhang but making support harder to remove.
    def = defs.add("support_tree_branch_distance", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Branch Distance");
    def->category = L("Support material");
    // TRN PrintSettings: "Organic supports" > "Branch Distance"
    def->tooltip = L("How far apart the branches need to be when they touch the model. "
                     "Making this distance small will cause the tree support to touch the model at more points, "
                     "causing better overhang but making support harder to remove.");
    def->mode = comAdvanced;
    SET_DEFAULT(1.);

    def = defs.add("support_tree_top_rate", Percent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Branch Density");
    def->category = L("Support material");
    // TRN PrintSettings: "Organic supports" > "Branch Density"
    def->tooltip = L("Adjusts the density of the support structure used to generate the tips of the branches. "
                     "A higher value results in better overhangs but the supports are harder to remove, "
                     "thus it is recommended to enable top support interfaces instead of a high branch density value "
                     "if dense interfaces are needed.");
    def->sidetext = L("%");
    def->min = 5;
    def->max_literal = 35;
    def->mode = comAdvanced;
    SET_DEFAULT(Percentage{15.});

    def = defs.add("temperature", Int);
    def->location = "filament_settings";
    def->label = L("Other layers");
    def->tooltip = L("Nozzle temperature for layers after the first one. Set this to zero to disable "
                     "temperature control commands in the output G-code.");
    def->sidetext = L("°C");
    def->full_label = L("Nozzle temperature");
    def->min = 0;
    def->max = max_temp;
    SET_DEFAULT(200);

    def = defs.add("thick_bridges", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Thick bridges");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("If enabled, bridges are more reliable, can bridge longer distances, but may look worse. "
                     "If disabled, bridges look better but are reliable just for shorter bridged distances.");
    def->mode = comAdvanced;
    SET_DEFAULT(true);

    def = defs.add("thin_walls", Bool);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Detect thin walls");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Detect single-width walls (parts where two extrusions don't fit and we need "
                   "to collapse them into a single trace).");
    def->mode = comAdvanced;
    SET_DEFAULT(true);

    def = defs.add("toolchange_gcode", String);
    def->location = "printer_settings";
    def->label = L("Tool change G-code");
    def->tooltip = L("This custom code is inserted before every toolchange. Placeholder variables for all PrusaSlicer settings "
                     "as well as {toolchange_z}, {previous_extruder} and {next_extruder} can be used. When a tool-changing command "
                     "which changes to the correct extruder is included (such as T{next_extruder}), PrusaSlicer will emit no other such command. "
                     "It is therefore possible to script custom behaviour both before and after the toolchange.");
    def->multiline = true;
    def->full_width = true;
    def->height = 5;
    def->mode = comExpert;
    SET_DEFAULT("");

    def = defs.add("top_infill_extrusion_width", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Top solid infill");
    def->category = L("Extrusion Width");
    def->tooltip = L("Set this to a non-zero value to set a manual extrusion width for infill for top surfaces. "
                   "You may want to use thinner extrudates to fill all narrow regions and get a smoother finish. "
                   "If left zero, default extrusion width will be used if set, otherwise nozzle diameter will be used. "
                   "If expressed as percentage (for example 90%) it will be computed over layer height.");
    def->sidetext = L("mm or %");
    def->min = 0;
    def->max_literal = 50;
    def->mode = comAdvanced;
    def->ratio_over = "layer_height";
    SET_DEFAULT(FloatOrPercentage{0.});

    def = defs.add("top_solid_infill_speed", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Top solid infill");
    def->category = L("Speed");
    def->tooltip = L("Speed for printing top solid layers (it only applies to the uppermost "
                   "external layers and not to their internal solid layers). You may want "
                   "to slow down this to get a nicer surface finish. This can be expressed "
                   "as a percentage (for example: 80%) over the solid infill speed above. "
                   "Set to zero for auto.");
    def->sidetext = L("mm/s or %");
    def->ratio_over = "solid_infill_speed";
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(FloatOrPercentage{15.});

    def = defs.add("top_solid_layers", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    //TRN Print Settings: "Top solid layers"
    def->label = L_CONTEXT("Top", "Layers");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Number of solid layers to generate on top surfaces.");
    def->full_label = L("Top solid layers");
    def->min = 0;
    SET_DEFAULT(3);

    def = defs.add("top_solid_min_thickness", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L_CONTEXT("Top", "Layers");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("The number of top solid layers is increased above top_solid_layers if necessary to satisfy "
    				 "minimum thickness of top shell."
    				 " This is useful to prevent pillowing effect when printing with variable layer height.");
    def->full_label = L("Minimum top shell thickness");
    def->sidetext = L("mm");
    def->min = 0;
    SET_DEFAULT(0.);

    def = defs.add("travel_speed", Double);
    def->location = "print_settings";
    def->label = L("Travel");
    def->tooltip = L("Speed for travel moves (jumps between distant extrusion points).");
    def->sidetext = L("mm/s");
    def->aliases = { "travel_feed_rate" };
    def->min = 1;
    def->mode = comAdvanced;
    SET_DEFAULT(130.);

    def = defs.add("travel_speed_z", Double);
    def->location = "print_settings";
    def->label = L("Z travel");
    def->tooltip = L("Speed for movements along the Z axis.\nWhen set to zero, the value "
                     "is ignored and regular travel speed is used instead.");
    def->sidetext = L("mm/s");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("use_firmware_retraction", Bool);
    def->location = "printer_settings";
    def->label = L("Use firmware retraction");
    def->tooltip = L("This setting uses G10 and G11 commands to have the firmware "
                   "handle the retraction. Note that this has to be supported by firmware.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("use_relative_e_distances", Bool);
    def->location = "printer_settings";
    def->label = L("Use relative E distances");
    def->tooltip = L("If your firmware requires relative E values, check this, "
                   "otherwise leave it unchecked. Most firmwares use absolute values.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("use_volumetric_e", Bool);
    def->location = "printer_settings";
    def->label = L("Use volumetric E");
    def->tooltip = L("This experimental setting uses outputs the E values in cubic millimeters "
                   "instead of linear millimeters. If your firmware doesn't already know "
                   "filament diameter(s), you can put commands like 'M200 D[filament_diameter_0] T0' "
                   "in your start G-code in order to turn volumetric mode on and use the filament "
                   "diameter associated to the filament selected in Slic3r. This is only supported "
                   "in recent Marlin.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("variable_layer_height", Bool);
    def->location = "printer_settings";
    def->label = L("Enable variable layer height feature");
    def->tooltip = L("Some printers or printer setups may have difficulties printing "
                   "with a variable layer height. Enabled by default.");
    def->mode = comExpert;
    SET_DEFAULT(true);

    def = defs.add("prefer_clockwise_movements", Bool);
    def->location = "printer_settings";
    def->label = L("Prefer clockwise movements");
    def->tooltip = L("This setting makes the printer print loops clockwise instead of counterclockwise.");
    def->mode = comExpert;
    SET_DEFAULT(false);

    def = defs.add("wipe", Bool);
    def->location = "toolprint_settings";
    def->overrides_in = { "filament_settings" };
    def->label = L("Wipe while retracting");
    def->tooltip = L("This flag will move the nozzle while retracting to minimize the possible blob "
                   "on leaky extruders.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("wipe_tower", Bool);
    def->location = "print_settings";
    def->label = L("Enable");
    def->tooltip = L("Multi material printers may need to prime or purge extruders on tool changes. "
                   "Extrude the excess material into the wipe tower.");
    def->mode = comAdvanced;
    SET_DEFAULT(false);

    def = defs.add("wiping_volumes_matrix", Doubles);
    def->location = "project_settings";
    def->label = L("Purging volumes - matrix");
    def->tooltip = L("This matrix describes volumes (in cubic milimetres) required to purge the"
                     " new filament on the wipe tower for any given pair of tools.");
    SET_DEFAULT((std::vector<double>
                {   0., 140., 140., 140., 140.,
                  140.,   0., 140., 140., 140.,
                  140., 140.,   0., 140., 140.,
                  140., 140., 140.,   0., 140.,
                  140., 140., 140., 140.,   0. }));

    def = defs.add("wiping_volumes_use_custom_matrix", Bool);
    def->location = "project_settings";
    def->label = "";
    def->tooltip = "";
    SET_DEFAULT(false);

    def = defs.add("wipe_tower_width", Double);
    def->location = "print_settings";
    def->label = L("Width");
    def->tooltip = L("Width of a wipe tower");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    SET_DEFAULT(60.);

    def = defs.add("wipe_tower_brim_width", Double);
    def->location = "print_settings";
    def->label = L("Wipe tower brim width");
    def->tooltip = L("Wipe tower brim width");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    def->min = 0.;
    SET_DEFAULT(2.);

    def = defs.add("wipe_tower_cone_angle", Double);
    def->location = "print_settings";
    def->label = L("Stabilization cone apex angle");
    def->tooltip = L("Angle at the apex of the cone that is used to stabilize the wipe tower. "
                     "Larger angle means wider base.");
    def->sidetext = L("°");
    def->mode = comAdvanced;
    def->min = 0.;
    def->max = 90.;
    SET_DEFAULT(0.);

    def = defs.add("wipe_tower_extra_spacing", Percent);
    def->location = "print_settings";
    def->label = L("Wipe tower purge lines spacing");
    def->tooltip = L("Spacing of purge lines on the wipe tower.");
    def->sidetext = L("%");
    def->mode = comExpert;
    def->min = 100.;
    def->max = 300.;
    SET_DEFAULT(Percentage{100.});

    def = defs.add("wipe_tower_extra_flow", Percent);
    def->location = "print_settings";
    def->label = L("Extra flow for purging");
    def->tooltip = L("Extra flow used for the purging lines on the wipe tower. This makes the purging lines thicker or narrower "
                     "than they normally would be. The spacing is adjusted automatically.");
    def->sidetext = L("%");
    def->mode = comExpert;
    def->min = 100.;
    def->max = 300.;
    SET_DEFAULT(Percentage{100.});

    def = defs.add("wipe_into_infill", Bool);
    def->location = "volume_settings";
    def->category = L("Wipe options");
    def->label = L("Wipe into this object's infill");
    def->tooltip = L("Purging after toolchange will be done inside this object's infills. "
                     "This lowers the amount of waste but may result in longer print time "
                     " due to additional travel moves.");
    SET_DEFAULT(false);

    def = defs.add("wipe_into_objects", Bool);
    def->location = "object_settings";
    def->category = L("Wipe options");
    def->label = L("Wipe into this object");
    def->tooltip = L("Object will be used to purge the nozzle after a toolchange to save material "
                     "that would otherwise end up in the wipe tower and decrease print time. "
                     "Colours of the objects will be mixed as a result.");
    SET_DEFAULT(false);

    def = defs.add("wipe_tower_bridging", Double);
    def->location = "print_settings";
    def->label = L("Maximal bridging distance");
    def->tooltip = L("Maximal distance between supports on sparse infill sections.");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    SET_DEFAULT(10.);

    def = defs.add("wipe_tower_extruder", Int);
    def->location = "print_settings";
    def->label = L("Wipe tower extruder");
    def->category = L("Extruders");
    def->tooltip = L("The extruder to use when printing perimeter of the wipe tower. "
                     "Set to 0 to use the one that is available (non-soluble would be preferred).");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0);

    def = defs.add("xy_size_compensation", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("XY Size Compensation");
    def->category = L("Advanced");
    def->tooltip = L("The object will be grown/shrunk in the XY plane by the configured value "
                   "(negative = inwards, positive = outwards). This might be useful "
                   "for fine-tuning hole sizes.");
    def->sidetext = L("mm");
    def->mode = comExpert;
    SET_DEFAULT(0.);
    
    def = defs.add("z_offset", Double);
    def->location = "printer_settings";
    def->label = L("Z offset");
    def->tooltip = L("This value will be added (or subtracted) from all the Z coordinates "
                   "in the output G-code. It is used to compensate for bad Z endstop position: "
                   "for example, if your endstop zero actually leaves the nozzle 0.3mm far "
                   "from the print bed, set this to -0.3 (or fix your endstop).");
    def->sidetext = L("mm");
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("perimeter_generator", Enum);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Perimeter generator");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("Classic perimeter generator produces perimeters with constant extrusion width and for "
                      "very thin areas is used gap-fill. "
                      "Arachne engine produces perimeters with variable extrusion width. "
                      "This setting also affects the Concentric infill.");
    def->enum_type = PerimeterGeneratorType::Classic;
    def->enum_values = { { int(PerimeterGeneratorType::Classic), "classic", L("Classic") },
                         { int(PerimeterGeneratorType::Arachne), "arachne", L("Arachne") } };

    def->mode = comAdvanced;
    SET_DEFAULT(PerimeterGeneratorType::Arachne);
    
    def = defs.add("wall_transition_length", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Perimeter transition length");
    def->category = L("Advanced");
    def->tooltip  = L("When transitioning between different numbers of perimeters as the part becomes "
                       "thinner, a certain amount of space is allotted to split or join the perimeter segments. "
                       "If expressed as a percentage (for example 100%), it will be computed based on the nozzle diameter.");
    def->sidetext = L("mm or %");
    def->mode = comExpert;
    def->min = 0;
    SET_DEFAULT(FloatOrPercentage(Percentage{100.}));

    def = defs.add("wall_transition_filter_deviation", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Perimeter transitioning filter margin");
    def->category = L("Advanced");
    def->tooltip  = L("Prevent transitioning back and forth between one extra perimeter and one less. This "
                       "margin extends the range of extrusion widths which follow to [Minimum perimeter width "
                       "- margin, 2 * Minimum perimeter width + margin]. Increasing this margin "
                       "reduces the number of transitions, which reduces the number of extrusion "
                       "starts/stops and travel time. However, large extrusion width variation can lead to "
                       "under- or overextrusion problems. "
                       "If expressed as a percentage (for example 25%), it will be computed based on the nozzle diameter.");
    def->sidetext = L("mm or %");
    def->mode = comExpert;
    def->min = 0;
    SET_DEFAULT(FloatOrPercentage(Percentage{25.}));

    def = defs.add("wall_transition_angle", Double);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Perimeter transitioning threshold angle");
    def->category = L("Advanced");
    def->tooltip  = L("When to create transitions between even and odd numbers of perimeters. A wedge shape with"
                       " an angle greater than this setting will not have transitions and no perimeters will be "
                       "printed in the center to fill the remaining space. Reducing this setting reduces "
                       "the number and length of these center perimeters, but may leave gaps or overextrude.");
    def->sidetext = L("°");
    def->mode = comExpert;
    def->min = 1.;
    def->max = 59.;
    SET_DEFAULT(10.);

    def = defs.add("wall_distribution_count", Int);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Perimeter distribution count");
    def->category = L("Advanced");
    def->tooltip  = L("The number of perimeters, counted from the center, over which the variation needs to be "
                       "spread. Lower values mean that the outer perimeters don't change in width.");
    def->mode = comExpert;
    def->min = 1;
    SET_DEFAULT(1);

    def = defs.add("min_feature_size", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Minimum feature size");
    def->category = L("Advanced");
    def->tooltip  = L("Minimum thickness of thin features. Model features that are thinner than this value will "
                       "not be printed, while features thicker than the Minimum feature size will be widened to "
                       "the Minimum perimeter width. "
                       "If expressed as a percentage (for example 25%), it will be computed based on the nozzle diameter.");
    def->sidetext = L("mm or %");
    def->mode = comExpert;
    def->min = 0;
    SET_DEFAULT(FloatOrPercentage(Percentage{25.}));

    def = defs.add("min_bead_width", FloatOrPercent);
    def->location = "print_settings";
    def->overrides_in = { "object_settings" };
    def->label = L("Minimum perimeter width");
    def->category = L("Advanced");
    def->tooltip  = L("Width of the perimeter that will replace thin features (according to the Minimum feature size) "
                       "of the model. If the Minimum perimeter width is thinner than the thickness of the feature,"
                       " the perimeter will become as thick as the feature itself. "
                       "If expressed as a percentage (for example 85%), it will be computed based on the nozzle diameter.");
    def->sidetext = L("mm or %");
    def->mode = comExpert;
    def->min = 0;
    SET_DEFAULT(FloatOrPercentage(Percentage{85.}));

    def = defs.add("idle_temperature", IntOptional);
    def->location = "filament_settings";
    def->label = L("Idle temperature");
    def->tooltip = L("Nozzle temperature when the tool is currently not used in multi-tool setups."
                     "This is only used when 'Ooze prevention' is active in Print Settings.");
    def->sidetext = L("°C");
    def->min = 0;
    def->max = max_temp;
    SET_DEFAULT(std::optional<int>());

}

} // namespace Slic3r::Domain
