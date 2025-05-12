#include "ConfigCommon.hpp"
#include "Slic3r/Domain/Config.hpp"

namespace Slic3r::Domain {


// JUST TEMPORARY UNTIL WE DECIDE WHAT TO DO WITH MODES.
// Right now, let's just define the constants so the defs compile.
enum { comSimple, comAdvanced, comExpert };

// Little helper to save some typing:
#define SET_DEFAULT(v) def->init_fn = [](ConfigItem& item) { item.set(v); };

// Define our own marking functions, the regular ones are not accessible in Domain.
static const std::string& L(const std::string& s) { return s; }
static const std::string& L_CONTEXT(const std::string& s, const std::string& ctx) { return s; }

void init_common_fdm_sla_config_items(ConfigDefinitions& defs, const std::string& technology)
{
	ConfigItemDef* def = nullptr;

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

    def = defs.add("printer_technology", Enum);
    def->location = technology == "FDM" ? "printer_settings" : "sla_printer_settings";
    def->label = L("Printer technology");
    def->tooltip = L("Printer technology");
    def->enum_type = PrinterTechnology::FFF;
    def->enum_values = { { int(PrinterTechnology::FFF), "FFF", L("FFF") },
                         { int(PrinterTechnology::SLA), "SLA", L("SLA") } };
    if (technology == "SLA")
        def->init_fn = [](ConfigItem& item) { item.set(PrinterTechnology::SLA); };
    else
        def->init_fn = [](ConfigItem& item) { item.set(PrinterTechnology::FFF); };

    def = defs.add("bed_shape", Points);
    def->location = technology == "FDM" ? "printer_settings" : "sla_printer_settings";
    def->label = L("Bed shape");
    def->mode = comAdvanced;
    SET_DEFAULT((std::vector<Domain::Vec2d>{{0., 0.}, { 200., 0. }, { 200., 200. }, { 0., 200. }}));

    def = defs.add("bed_custom_texture", String);
    def->location = technology == "FDM" ? "printer_settings" : "sla_printer_settings";
    def->label = L("Bed custom texture");
    def->mode = comAdvanced;
    SET_DEFAULT("");

    def = defs.add("bed_custom_model", String);
    def->location = technology == "FDM" ? "printer_settings" : "sla_printer_settings";
    def->label = L("Bed custom model");
    def->mode = comAdvanced;
    SET_DEFAULT("");

    def = defs.add("elefant_foot_compensation", Double);
    def->location = technology == "FDM" ? "print_settings" : "sla_print_settings";
    if (technology == "SLA")
        def->overrides_in = { "sla_material_settings", "sla_object_settings"};
    if (technology == "FDM")
        def->overrides_in = { "object_settings", "volume_settings"};
    def->label = L("Elephant foot compensation");
    def->category = L("Advanced");
    def->tooltip = L("The first layer will be shrunk in the XY plane by the configured value "
                     "to compensate for the 1st layer squish aka an Elephant Foot effect.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.);

    def = defs.add("thumbnails", String);
    def->location = technology == "FDM" ? "printer_settings" : "sla_printer_settings";
    def->label = L("G-code thumbnails");
    def->tooltip = L("Picture sizes to be stored into a .gcode / .bgcode and .sl1 / .sl1s files, in the following format: \"XxY/EXT, XxY/EXT, ...\"\n"
                     "Currently supported extensions are PNG, QOI and JPG.");
    def->mode = comExpert;
    def->gui_type = ConfigItemDef::GUIType::one_string;
    SET_DEFAULT("");

    def = defs.add("thumbnails_format", Enum);
    def->location = technology == "FDM" ? "printer_settings" : "sla_printer_settings";
    def->label = L("Format of G-code thumbnails");
    def->tooltip = L("Format of G-code thumbnails: PNG for best quality, JPG for smallest size, QOI for low memory firmware");
    def->mode = comExpert;
    def->enum_type = GCodeThumbnailsFormat::PNG;
    def->enum_values = { { int(GCodeThumbnailsFormat::PNG), "PNG", "PNG" },
                         { int(GCodeThumbnailsFormat::JPG), "JPG", "JPG" },
                         { int(GCodeThumbnailsFormat::QOI), "QOI", "QOI" } };
    SET_DEFAULT(GCodeThumbnailsFormat::PNG);

    def = defs.add("layer_height", Double);
    def->location = technology == "FDM" ? "print_settings" : "sla_print_settings";
    if (technology == "FDM")
        def->overrides_in = { "object_settings", "volume_settings" };
    def->label = L("Layer height");
    def->category = L("Layers and Perimeters");
    def->tooltip = L("This setting controls the height (and thus the total number) of the slices/layers. "
                   "Thinner layers give better accuracy but take more time to print.");
    def->sidetext = L("mm");
    def->min = 0;
    SET_DEFAULT(0.3);

    def = defs.add("max_print_height", Double);
    def->location = technology == "FDM" ? "print_settings" : "sla_print_settings";
    def->label = L("Max print height");
    def->tooltip = L("Set this to the maximum height that can be reached by your extruder while printing.");
    def->sidetext = L("mm");
    def->min = 0;
    def->max = 1200;
    def->mode = comAdvanced;
    SET_DEFAULT(200.);

    def = defs.add("output_filename_format", String);
    def->location = technology == "FDM" ? "print_settings" : "sla_print_settings";
    def->label = L("Output filename format");
    def->tooltip = L("You can use all configuration options as variables inside this template. "
                   "For example: [layer_height], [fill_density] etc. You can also use [timestamp], "
                   "[year], [month], [day], [hour], [minute], [second], [version], "
                   "[input_filename_base], [default_output_extension].");
    def->full_width = true;
    def->mode = comExpert;
    SET_DEFAULT("[input_filename_base].gcode");

    def = defs.add("slice_closing_radius", Double);
    def->location = technology == "FDM" ? "print_settings" : "sla_print_settings";
    if (technology == "FDM")
        def->overrides_in = {"object_settings", "volume_settings" };
    def->label = L("Slice gap closing radius");
    def->category = L("Advanced");
    def->tooltip = L("Cracks smaller than 2x gap closing radius are being filled during the triangle mesh slicing. "
                     "The gap closing operation may reduce the final print resolution, therefore it is advisable to keep the value reasonably low.");
    def->sidetext = L("mm");
    def->min = 0;
    def->mode = comAdvanced;
    SET_DEFAULT(0.049);

}

}