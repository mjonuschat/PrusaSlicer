#include "Slic3r/Domain/ConfigSLA.hpp"
#include "ConfigCommon.hpp"

#include "Slic3r/Domain/Types.hpp"

#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"

namespace Slic3r::Domain {

// Implementation of SLA configs is done in this file.

// Define our own marking functions, the regular ones are not accessible in Domain.
static const std::string& L(const std::string& s) { return s; }
static const std::string& L_CONTEXT(const std::string& s, const std::string& ctx) { return s; }

void sla_config_init_fn(ConfigDefinitions& defs);

// Define the static object holding all definitions. Provide list of acceptable
// boxes and the init function.
ConfigDefinitions s_defs_sla({"sla_printer_settings", "sla_print_settings", "sla_material_settings",
    "sla_object_settings", "sla_volume_settings"}, sla_config_init_fn);


// JUST TEMPORARY UNTIL WE DECIDE WHAT TO DO WITH MODES.
// Right now, let's just define the constants so the defs compile.
enum { comSimple, comAdvanced, comExpert };



// Little helper to save some typing:
#define SET_DEFAULT(v) def->init_fn = [](ConfigItem& item) { item.set(v); };

// Now define the init function. This function will be called by ConfigDefinitions
// constructor and will fill the definitions with all the necessary data.
void sla_config_init_fn(ConfigDefinitions& defs)
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

    /* TODO: These were common for FDM and SLA. Do we want to 
    separate them or keep them together? They are not defined here
    at this point.
    def = defs.add("printer_technology", Enum);
    def = defs.add("bed_shape", Points);
    def = defs.add("bed_custom_texture", String);
    def = defs.add("bed_custom_model", String);
    def = defs.add("elefant_foot_compensation", Double); - has an override in sla_material_settings
    def = defs.add("thumbnails", String);
    def = defs.add("thumbnails_format", Enum);
    def = defs.add("layer_height", Double);
    def = defs.add("max_print_height", Double);*/

    init_common_fdm_sla_config_items(defs, "SLA");


    def = defs.add("display_width", Double);
    def->location = "sla_printer_settings";
    def->label = L("Display width");
    def->tooltip = L("Width of the display");
    def->min = 1;
    SET_DEFAULT(120.);

    def = defs.add("display_height", Double);
    def->location = "sla_printer_settings";
    def->label = L("Display height");
    def->tooltip = L("Height of the display");
    def->min = 1;
    SET_DEFAULT(68.);

    def = defs.add("display_pixels_x", Int);
    def->location = "sla_printer_settings";
    def->full_label = L("Number of pixels in");
    def->label = ("X");
    def->tooltip = L("Number of pixels in X");
    def->min = 100;
    SET_DEFAULT(2560);

    def = defs.add("display_pixels_y", Int);
    def->location = "sla_printer_settings";
    def->label = ("Y");
    def->tooltip = L("Number of pixels in Y");
    def->min = 100;
    SET_DEFAULT(1440);

    def = defs.add("display_mirror_x", Bool);
    def->location = "sla_printer_settings";
    def->full_label = L("Display horizontal mirroring");
    def->label = L("Mirror horizontally");
    def->tooltip = L("Enable horizontal mirroring of output images");
    def->mode = comExpert;
    SET_DEFAULT(true);

    def = defs.add("display_mirror_y", Bool);
    def->location = "sla_printer_settings";
    def->full_label = L("Display vertical mirroring");
    def->label = L("Mirror vertically");
    def->tooltip = L("Enable vertical mirroring of output images");
    def->mode = comExpert;
    SET_DEFAULT(false);
    
    def = defs.add("display_orientation", Enum);
    def->location = "sla_printer_settings";
    def->label = L("Display orientation");
    def->tooltip = L("Set the actual LCD display orientation inside the SLA printer."
                     " Portrait mode will flip the meaning of display width and height parameters"
                     " and the output images will be rotated by 90 degrees.");
    def->enum_type = SLADisplayOrientation::sladoLandscape;
    def->enum_values = { { int(SLADisplayOrientation::sladoLandscape), "landscape", L("Landscape") },
                         { int(SLADisplayOrientation::sladoPortrait),  "portrait",  L("Portrait")  } };
    def->mode = comExpert;
    SET_DEFAULT(SLADisplayOrientation::sladoPortrait);

    def = defs.add("fast_tilt_time", Double);
    def->location = "sla_printer_settings";
    def->label = L("Fast");
    def->full_label = L("Fast tilt");
    def->tooltip = L("Time of the fast tilt");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(5.);

    def = defs.add("slow_tilt_time", Double);
    def->location = "sla_printer_settings";
    def->label = L("Slow");
    def->full_label = L("Slow tilt");
    def->tooltip = L("Time of the slow tilt");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(8.);

    def = defs.add("high_viscosity_tilt_time", Double);
    def->location = "sla_printer_settings";
    def->label = L("High viscosity");
    def->full_label = L("Tilt for high viscosity resin");
    def->tooltip = L("Time of the super slow tilt");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(10.);

    def = defs.add("area_fill", Double);
    def->location = "sla_material_settings";
    def->label = L("Area fill threshold");
    def->tooltip = L("The value is expressed as a percentage of the bed area. If the area of a particular layer "
                     "is smaller than 'area_fill', then 'Below area fill threshold' parameters are used to determine the "
                     "layer separation (tearing) procedure. Otherwise 'Above area fill threshold' parameters are used.");
    def->sidetext = L("%");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(35.);

    def = defs.add("relative_correction", Doubles);
    def->location = "sla_printer_settings";
    def->label = L("Printer scaling correction");
    def->full_label = L("Printer scaling correction");
    def->tooltip  = L("Printer scaling correction");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT((std::vector<double>{1.,1.}));

    def = defs.add("relative_correction_x", Double);
    def->location = "sla_printer_settings";
    def->label = L("Printer scaling correction in X axis");
    def->full_label = L("Printer scaling X axis correction");
    def->tooltip  = L("Printer scaling correction in X axis");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(1.);

    def = defs.add("relative_correction_y", Double);
    def->location = "sla_printer_settings";
    def->label = L("Printer scaling correction in Y axis");
    def->full_label = L("Printer scaling Y axis correction");
    def->tooltip  = L("Printer scaling correction in Y axis");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(1.);

    def = defs.add("relative_correction_z", Double);
    def->location = "sla_printer_settings";
    def->label = L("Printer scaling correction in Z axis");
    def->full_label = L("Printer scaling Z axis correction");
    def->tooltip  = L("Printer scaling correction in Z axis");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(1.);

    def = defs.add("absolute_correction", Double);
    def->location = "sla_printer_settings";
    def->overrides_in = { "sla_material_settings" };
    def->label = L("Printer absolute correction");
    def->full_label = L("Printer absolute correction");
    def->tooltip  = L("Will inflate or deflate the sliced 2D polygons according "
                      "to the sign of the correction.");
    def->sidetext = L("mm");
    def->mode = comExpert;
    SET_DEFAULT(0.);
    
    def = defs.add("elefant_foot_min_width", Double);
    def->location = "sla_printer_settings";
    def->label = L("Elephant foot minimum width");
    def->category = L("Advanced");
    def->tooltip = L("Minimum width of features to maintain when doing elephant foot compensation.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.2);

    def = defs.add("zcorrection_layers", Int);
    def->location = "sla_material_settings";
    def->label = L("Z compensation");
    def->category = L("Advanced");
    def->tooltip = L("Number of layers to Z correct to avoid cross layer bleed");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0);

    def = defs.add("gamma_correction", Double);
    def->location = "sla_printer_settings";
    def->label = L("Printer gamma correction");
    def->full_label = L("Printer gamma correction");
    def->tooltip  = L("This will apply a gamma correction to the rasterized 2D "
                      "polygons. A gamma value of zero means thresholding with "
                      "the threshold in the middle. This behaviour eliminates "
                      "antialiasing without losing holes in polygons.");
    def->min = 0;
    def->max = 1;
    def->mode = comExpert;
    SET_DEFAULT(1.);


    def = defs.add("material_colour", String);
    def->location = "sla_material_settings";
    def->label = L("Color");
    def->tooltip = L("This is only used in the Slic3r interface as a visual help.");
    def->gui_type = ConfigItemDef::GUIType::color;
    SET_DEFAULT("#29B2B2");

    def = defs.add("material_type", String);
    def->location = "sla_material_settings";
    def->label = L("SLA material type");
    def->tooltip = L("SLA material type");
    def->gui_flags = "show_value";
    def->choices = {
        { std::string("Tough"),  std::string("Tough")   },
        { std::string("Flexible"),  std::string("Flexible")   },
        { std::string("Casting"),  std::string("Casting")   },
        { std::string("Dental"),  std::string("Dental")   },
        { std::string("Heat-resistant"),  std::string("Heat-resistant")   } };
    SET_DEFAULT("Tough");

    def = defs.add("initial_layer_height", Double);
    def->location = "sla_material_settings";
    def->label = L("Initial layer height");
    def->tooltip = L("Initial layer height");
    def->sidetext = L("mm");
    def->min = 0;
    SET_DEFAULT(0.3);

    def = defs.add("bottle_volume", Double);
    def->location = "sla_material_settings";
    def->label = L("Bottle volume");
    def->tooltip = L("Bottle volume");
    def->sidetext = L("ml");
    def->min = 50;
    SET_DEFAULT(1000.);

    def = defs.add("bottle_weight", Double);
    def->location = "sla_material_settings";
    def->label = L("Bottle weight");
    def->tooltip = L("Bottle weight");
    def->sidetext = L("kg");
    def->min = 0;
    SET_DEFAULT(1.);

    def = defs.add("material_density", Double);
    def->location = "sla_material_settings";
    def->label = L("Density");
    def->tooltip = L("Density");
    def->sidetext = L("g/ml");
    def->min = 0;
    SET_DEFAULT(1.);

    def = defs.add("bottle_cost", Double);
    def->location = "sla_material_settings";
    def->label = L("Cost");
    def->tooltip = L("Cost");
    def->sidetext = L("money/bottle");
    def->min = 0;
    SET_DEFAULT(0.);

    def = defs.add("faded_layers", Int);
    def->location = "sla_print_settings";
    def->label = L("Faded layers");
    def->tooltip = L("Number of the layers needed for the exposure time fade from initial exposure time to the exposure time");
    def->min = 3;
    def->max = 20;
    def->mode = comExpert;
    SET_DEFAULT(10);

    def = defs.add("min_exposure_time", Double);
    def->location = "sla_printer_settings";
    def->label = L("Minimum exposure time");
    def->tooltip = L("Minimum exposure time");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("max_exposure_time", Double);
    def->location = "sla_printer_settings";
    def->label = L("Maximum exposure time");
    def->tooltip = L("Maximum exposure time");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(100.);

    def = defs.add("exposure_time", Double);
    def->location = "sla_material_settings";
    def->label = L("Exposure time");
    def->tooltip = L("Exposure time");
    def->sidetext = L("s");
    def->min = 0;
    SET_DEFAULT(10.);

    def = defs.add("min_initial_exposure_time", Double);
    def->location = "sla_printer_settings";
    def->label = L("Minimum initial exposure time");
    def->tooltip = L("Minimum initial exposure time");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.);

    def = defs.add("max_initial_exposure_time", Double);
    def->location = "sla_printer_settings";
    def->label = L("Maximum initial exposure time");
    def->tooltip = L("Maximum initial exposure time");
    def->sidetext = L("s");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(150.);

    def = defs.add("initial_exposure_time", Double);
    def->location = "sla_material_settings";
    def->label = L("Initial exposure time");
    def->tooltip = L("Initial exposure time");
    def->sidetext = L("s");
    def->min = 0;
    SET_DEFAULT(15.);

    def = defs.add("material_correction", Doubles);
    def->location = "sla_material_settings";
    def->full_label = L("Correction for expansion");
    def->tooltip  = L("Correction for expansion");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT((std::vector<double>{ 1., 1., 1. }));

    def = defs.add("material_correction_x", Double);
    def->location = "sla_material_settings";
    def->full_label = L("Correction for expansion in X axis");
    def->tooltip  = L("Correction for expansion in X axis");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(1.);

    def = defs.add("material_correction_y", Double);
    def->location = "sla_material_settings";
    def->full_label = L("Correction for expansion in Y axis");
    def->tooltip  = L("Correction for expansion in Y axis");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(1.);

    def = defs.add("material_correction_z", Double);
    def->location = "sla_material_settings";
    def->full_label = L("Correction for expansion in Z axis");
    def->tooltip  = L("Correction for expansion in Z axis");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(1.);

    def = defs.add("material_notes", String);
    def->location = "sla_material_settings";
    def->label = L("SLA print material notes");
    def->tooltip = L("You can put your notes regarding the SLA print material here.");
    def->multiline = true;
    def->full_width = true;
    def->height = 13;
    // TODO currently notes are the only way to pass data
    // for non-PrusaResearch printers. We therefore need to always show them 
    def->mode = comSimple;
    SET_DEFAULT("");

    /* TODO: what about this?
    def = defs.add("material_vendor", coString);
    def->set_default_value(new ConfigOptionString(L("(Unknown)")));
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("default_sla_material_profile", coString);
    def->label = L("Default SLA material profile");
    def->tooltip = L("Default print profile associated with the current printer profile. "
                   "On selection of the current printer profile, this print profile will be activated.");
    def->set_default_value(new ConfigOptionString());
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("sla_material_settings_id", coString);
    def->set_default_value(new ConfigOptionString(""));
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("default_sla_print_profile", coString);
    def->label = L("Default SLA material profile");
    def->tooltip = L("Default print profile associated with the current printer profile. "
                   "On selection of the current printer profile, this print profile will be activated.");
    def->set_default_value(new ConfigOptionString());
    def->cli = ConfigOptionDef::nocli;

    def = defs.add("sla_print_settings_id", coString);
    def->set_default_value(new ConfigOptionString(""));
    def->cli = ConfigOptionDef::nocli;*/

    def = defs.add("supports_enable", Bool);
    def->location = "sla_print_settings";
    def->label = L("Generate supports");
    def->category = L("Supports");
    def->tooltip = L("Generate supports for the models");
    def->mode = comSimple;
    SET_DEFAULT(true);

    def = defs.add("support_tree_type", Enum);
    def->location = "sla_print_settings";
    def->label = L("Support tree type");
    def->tooltip = L("Support tree building strategy");
    def->enum_type = sla::SupportTreeType::Default;
    def->enum_values = { { int(sla::SupportTreeType::Default), "default", L("Default") },
                           // TRN One of the "Support tree type"s on SLAPrintSettings : Supports
                         { int(sla::SupportTreeType::Branching),  "branching",  L("Branching (experimental)")  },
                         // TODO: { int(sla::SupportTreeType::Organic),  "organic",  L("Organic")  }
                       };
    def->mode = comSimple;
    SET_DEFAULT(sla::SupportTreeType::Default);

    def = defs.add("support_enforcers_only", Bool);
    def->location = "sla_print_settings";
    def->label = L("Support only in enforced regions");
    def->category = L("Supports");
    def->tooltip = L("Only create support if it lies in a support enforcer.");
    def->mode = comSimple;
    SET_DEFAULT(false);

    def = defs.add("support_points_density_relative", Int);
    def->location = "sla_material_settings";
    def->overrides_in = { "sla_material_settings" };
    def->label = L("Support points density");
    def->category = L("Supports");
    def->tooltip = L("This is a relative measure of support points density.");
    def->sidetext = L("%");
    def->min = 0;
    SET_DEFAULT(100);

    def = defs.add("pad_enable", Bool);
    def->location = "sla_print_settings";
    def->label = L("Use pad");
    def->category = L("Pad");
    def->tooltip = L("Add a pad underneath the supported model");
    def->mode = comSimple;
    SET_DEFAULT(true);

    def = defs.add("pad_wall_thickness", Double);
    def->location = "sla_print_settings";
    def->label = L("Pad wall thickness");
    def->category = L("Pad");
     def->tooltip = L("The thickness of the pad and its optional cavity walls.");
    def->sidetext = L("mm");
    def->min = 0;
    def->max = 30;
    def->mode = comSimple;
    SET_DEFAULT(2.);

    def = defs.add("pad_wall_height", Double);
    def->location = "sla_print_settings";
    def->label = L("Pad wall height");
    def->tooltip = L("Defines the pad cavity depth. Set to zero to disable the cavity. "
                     "Be careful when enabling this feature, as some resins may "
                     "produce an extreme suction effect inside the cavity, "
                     "which makes peeling the print off the vat foil difficult.");
    def->category = L("Pad");
//     def->tooltip = L("");
    def->sidetext = L("mm");
    def->min = 0;
    def->max = 30;
    def->mode = comExpert;
    SET_DEFAULT(0.);
    
    def = defs.add("pad_brim_size", Double);
    def->location = "sla_print_settings";
    def->label = L("Pad brim size");
    def->tooltip = L("How far should the pad extend around the contained geometry");
    def->category = L("Pad");
    //     def->tooltip = L("");
    def->sidetext = L("mm");
    def->min = 0;
    def->max = 30;
    def->mode = comAdvanced;
    SET_DEFAULT(1.6);

    def = defs.add("pad_max_merge_distance", Double);
    def->location = "sla_print_settings";
    def->label = L("Max merge distance");
    def->category = L("Pad");
     def->tooltip = L("Some objects can get along with a few smaller pads "
                      "instead of a single big one. This parameter defines "
                      "how far the center of two smaller pads should be. If they"
                      "are closer, they will get merged into one pad.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(50.);

    // This is disabled on the UI. I hope it will never be enabled.
//    def = defs.add("pad_edge_radius", Double);
//    def->label = L("Pad edge radius");
//    def->category = L("Pad");
////     def->tooltip = L("");
//    def->sidetext = L("mm");
//    def->min = 0;
//    def->mode = comAdvanced;
//    SET_DEFAULT(1.0));

    def = defs.add("pad_wall_slope", Double);
    def->location = "sla_print_settings";
    def->label = L("Pad wall slope");
    def->category = L("Pad");
    def->tooltip = L("The slope of the pad wall relative to the bed plane. "
                     "90 degrees means straight walls.");
    def->sidetext = L("°");
    def->min = 45;
    def->max = 90;
    def->mode = comAdvanced;
    SET_DEFAULT(90.);

    def = defs.add("pad_around_object", Bool);
    def->location = "sla_print_settings";
    def->label = L("Pad around object");
    def->category = L("Pad");
    def->tooltip = L("Create pad around object and ignore the support elevation");
    def->mode = comSimple;
    SET_DEFAULT(false);
    
    def = defs.add("pad_around_object_everywhere", Bool);
    def->location = "sla_print_settings";
    def->label = L("Pad around object everywhere");
    def->category = L("Pad");
    def->tooltip = L("Force pad around object everywhere");
    def->mode = comSimple;
    SET_DEFAULT(false);

    def = defs.add("pad_object_gap", Double);
    def->location = "sla_print_settings";
    def->label = L("Pad object gap");
    def->category = L("Pad");
    def->tooltip  = L("The gap between the object bottom and the generated "
                      "pad in zero elevation mode.");
    def->sidetext = L("mm");
    def->min = 0;
    def->max = 10;
    def->mode = comExpert;
    SET_DEFAULT(1.);

    def = defs.add("pad_object_connector_stride", Double);
    def->location = "sla_print_settings";
    def->label = L("Pad object connector stride");
    def->category = L("Pad");
    def->tooltip = L("Distance between two connector sticks which connect the object and the generated pad.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(10.);

    def = defs.add("pad_object_connector_width", Double);
    def->location = "sla_print_settings";
    def->label = L("Pad object connector width");
    def->category = L("Pad");
    def->tooltip  = L("Width of the connector sticks which connect the object and the generated pad.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.5);

    def = defs.add("pad_object_connector_penetration", Double);
    def->location = "sla_print_settings";
    def->label = L("Pad object connector penetration");
    def->category = L("Pad");
    def->tooltip  = L(
        "How much should the tiny connectors penetrate into the model body.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comExpert;
    SET_DEFAULT(0.3);
    
    def = defs.add("hollowing_enable", Bool);
    def->location = "sla_print_settings";
    def->label = L("Enable hollowing");
    def->category = L("Hollowing");
    def->tooltip = L("Hollow out a model to have an empty interior");
    def->mode = comSimple;
    SET_DEFAULT(false);
    
    def = defs.add("hollowing_min_thickness", Double);
    def->location = "sla_print_settings";
    def->label = L("Wall thickness");
    def->category = L("Hollowing");
    def->tooltip  = L("Minimum wall thickness of a hollowed model.");
    def->sidetext = L("mm");
    def->min = 1;
    def->max = 10;
    def->mode = comSimple;
    SET_DEFAULT(3.);
    
    def = defs.add("hollowing_quality", Double);
    def->location = "sla_print_settings";
    def->label = L("Accuracy");
    def->category = L("Hollowing");
    def->tooltip  = L("Performance vs accuracy of calculation. Lower values may produce unwanted artifacts.");
    def->min = 0;
    def->max = 1;
    def->mode = comExpert;
    SET_DEFAULT(0.5);
    
    def = defs.add("hollowing_closing_distance", Double);
    def->location = "sla_print_settings";
    def->label = L("Closing distance");
    def->category = L("Hollowing");
    def->tooltip  = L(
        "Hollowing is done in two steps: first, an imaginary interior is "
        "calculated deeper (offset plus the closing distance) in the object and "
        "then it's inflated back to the specified offset. A greater closing "
        "distance makes the interior more rounded. At zero, the interior will "
        "resemble the exterior the most.");
    def->sidetext = L("mm");
    def->min = 0;
    def->max = 10;
    def->mode = comExpert;
    SET_DEFAULT(2.);

    def = defs.add("material_print_speed", Enum);
    def->location = "sla_material_settings";
    def->label = L("Print speed");
    def->tooltip = L(
        "A slower printing profile might be necessary when using materials with higher viscosity "
        "or with some hollowed parts. It slows down the tilt movement and adds a delay before exposure.");
    def->enum_type = SLAMaterialSpeed::slamsSlow;
    def->enum_values = { { int(SLAMaterialSpeed::slamsSlow),           "slow",            L("Slow") },
                         { int(SLAMaterialSpeed::slamsFast),           "fast",            L("Fast") },
                         { int(SLAMaterialSpeed::slamsHighViscosity),  "high_viscosity",  L("High viscosity") } };
    def->mode = comAdvanced;
    SET_DEFAULT(SLAMaterialSpeed::slamsFast);

    def = defs.add("sla_archive_format", String);
    def->location = "sla_printer_settings";
    def->label = L("Format of the output SLA archive");
    def->mode = comAdvanced;
    SET_DEFAULT("SL1");

    def = defs.add("sla_output_precision", Double);
    def->location = "sla_printer_settings";
    def->label = L("SLA output precision");
    def->tooltip = L("Minimum resolution in nanometers");
    def->sidetext = L("mm");
    def->min = 0.000001f;
    def->mode = comExpert;
    SET_DEFAULT(0.001);

    for (const std::string& prefix : { "", "branching" }) {
        def = defs.add(prefix + "support_head_front_diameter", Double);
        def->location = "sla_print_settings";
        def->overrides_in = { "sla_material_settings" };
        def->label = L("Pinhead front diameter");
        def->category = L("Supports");
        def->tooltip = L("Diameter of the pointing side of the head");
        def->sidetext = L("mm");
        def->min = 0;
        def->mode = comAdvanced;
        SET_DEFAULT(0.4);

        def = defs.add(prefix + "support_head_penetration", Double);
        def->location = "sla_print_settings";
        def->overrides_in = { "sla_material_settings" };
        def->label = L("Head penetration");
        def->category = L("Supports");
        def->tooltip = L("How much the pinhead has to penetrate the model surface");
        def->sidetext = L("mm");
        def->mode = comAdvanced;
        def->min = 0;
        SET_DEFAULT(0.2);

        def = defs.add(prefix + "support_head_width", Double);
        def->location = "sla_print_settings";
        def->overrides_in = { "sla_material_settings" };
        def->label = L("Pinhead width");
        def->category = L("Supports");
        def->tooltip = L("Width from the back sphere center to the front sphere center");
        def->sidetext = L("mm");
        def->min = 0;
        def->max = 20;
        def->mode = comAdvanced;
        SET_DEFAULT(1.);

        def = defs.add(prefix + "support_pillar_diameter", Double);
        def->location = "sla_print_settings";
        def->overrides_in = { "sla_material_settings" };
        def->label = L("Pillar diameter");
        def->category = L("Supports");
        def->tooltip = L("Diameter in mm of the support pillars");
        def->sidetext = L("mm");
        def->min = 0;
        def->max = 15;
        def->mode = comSimple;
        SET_DEFAULT(1.);

        def = defs.add(prefix + "support_small_pillar_diameter_percent", Percent);
        def->location = "sla_print_settings";
        def->label = L("Small pillar diameter percent");
        def->category = L("Supports");
        def->tooltip = L("The percentage of smaller pillars compared to the normal pillar diameter "
            "which are used in problematic areas where a normal pilla cannot fit.");
        def->sidetext = L("%");
        def->min = 1;
        def->max = 100;
        def->mode = comExpert;
        SET_DEFAULT(Percentage(50.));

        def = defs.add(prefix + "support_max_bridges_on_pillar", Int);
        def->location = "sla_print_settings";
        def->label = L("Max bridges on a pillar");
        def->tooltip = L(
            "Maximum number of bridges that can be placed on a pillar. Bridges "
            "hold support point pinheads and connect to pillars as small branches.");
        def->min = 0;
        def->max = 50;
        def->mode = comExpert;
        if (prefix == "branching")
            def->init_fn = [](ConfigItem& item) { item.set(2); };
        else
            def->init_fn = [](ConfigItem& item) { item.set(3); };

        def = defs.add(prefix + "support_max_weight_on_model", Double);
        def->location = "sla_print_settings";
        def->label = L("Max weight on model");
        def->category = L("Supports");
        def->tooltip = L(
            "Maximum weight of sub-trees that terminate on the model instead of the print bed. The weight is the sum of the lenghts of all "
            "branches emanating from the endpoint.");
        def->sidetext = L("mm");
        def->min = 0;
        def->mode = comExpert;
        SET_DEFAULT(10.);

        def = defs.add(prefix + "support_pillar_connection_mode", Enum);
        def->location = "sla_print_settings";
        def->label = L("Pillar connection mode");
        def->tooltip = L("Controls the bridge type between two neighboring pillars."
            " Can be zig-zag, cross (double zig-zag) or dynamic which"
            " will automatically switch between the first two depending"
            " on the distance of the two pillars.");
        def->enum_type = sla::PillarConnectionMode::dynamic;
        def->enum_values = {
            { int(sla::PillarConnectionMode::zigzag),  "zigzag",  L("Zig-Zag") },
            { int(sla::PillarConnectionMode::cross),   "cross",   L("Cross")   },
            { int(sla::PillarConnectionMode::dynamic), "dynamic", L("Dynamic") }
        };
        def->mode = comAdvanced;
        SET_DEFAULT(sla::PillarConnectionMode::dynamic);

        def = defs.add(prefix + "support_buildplate_only", Bool);
        def->location = "sla_print_settings";
        def->label = L("Support on build plate only");
        def->category = L("Supports");
        def->tooltip = L("Only create support if it lies on a build plate. Don't create support on a print.");
        def->mode = comSimple;
        SET_DEFAULT(false);

        def = defs.add(prefix + "support_pillar_widening_factor", Double);
        def->location = "sla_print_settings";
        def->label = L("Pillar widening factor");
        def->category = L("Supports");
        def->tooltip =
            L("Merging bridges or pillars into another pillars can "
                "increase the radius. Zero means no increase, one means "
                "full increase. The exact amount of increase is unspecified and can "
                "change in the future.");
        def->min = 0;
        def->max = 1;
        def->mode = comExpert;
        SET_DEFAULT(0.5);

        def = defs.add(prefix + "support_base_diameter", Double);
        def->location = "sla_print_settings";
        def->label = L("Support base diameter");
        def->category = L("Supports");
        def->tooltip = L("Diameter in mm of the pillar base");
        def->sidetext = L("mm");
        def->min = 0;
        def->max = 30;
        def->mode = comAdvanced;
        SET_DEFAULT(4.);

        def = defs.add(prefix + "support_base_height", Double);
        def->location = "sla_print_settings";
        def->label = L("Support base height");
        def->category = L("Supports");
        def->tooltip = L("The height of the pillar base cone");
        def->sidetext = L("mm");
        def->min = 0;
        def->mode = comAdvanced;
        SET_DEFAULT(1.);

        def = defs.add(prefix + "support_base_safety_distance", Double);
        def->location = "sla_print_settings";
        def->label = L("Support base safety distance");
        def->category = L("Supports");
        def->tooltip = L(
            "The minimum distance of the pillar base from the model in mm. "
            "Makes sense in zero elevation mode where a gap according "
            "to this parameter is inserted between the model and the pad.");
        def->sidetext = L("mm");
        def->min = 0;
        def->max = 10;
        def->mode = comExpert;
        SET_DEFAULT(1.);

        def = defs.add(prefix + "support_critical_angle", Double);
        def->location = "sla_print_settings";
        def->label = L("Critical angle");
        def->category = L("Supports");
        def->tooltip = L("The default angle for connecting support sticks and junctions.");
        def->sidetext = L("°");
        def->min = 0;
        def->max = 90;
        def->mode = comExpert;
        SET_DEFAULT(45.);

        def = defs.add(prefix + "support_max_bridge_length", Double);
        def->location = "sla_print_settings";
        def->label = L("Max bridge length");
        def->category = L("Supports");
        def->tooltip = L("The max length of a bridge");
        def->sidetext = L("mm");
        def->min = 0;
        def->mode = comAdvanced;
        if (prefix == "branching")
            def->init_fn = [](ConfigItem& item) { item.set(5.); };
        else
            def->init_fn = [](ConfigItem& item) { item.set(15.); };

        def = defs.add(prefix + "support_max_pillar_link_distance", Double);
        def->location = "sla_print_settings";
        def->label = L("Max pillar linking distance");
        def->category = L("Supports");
        def->tooltip = L("The max distance of two pillars to get linked with each other."
            " A zero value will prohibit pillar cascading.");
        def->sidetext = L("mm");
        def->min = 0;   // 0 means no linking
        def->mode = comAdvanced;
        SET_DEFAULT(10.);

        def = defs.add(prefix + "support_object_elevation", Double);
        def->location = "sla_print_settings";
        def->label = L("Object elevation");
        def->category = L("Supports");
        def->tooltip = L("How much the supports should lift up the supported object. "
            "If \"Pad around object\" is enabled, this value is ignored.");
        def->sidetext = L("mm");
        def->min = 0;
        def->max = 150; // This is the max height of print on SL1
        def->mode = comAdvanced;
        SET_DEFAULT(5.);
    }
}



} // namespace Slic3r::Domain
