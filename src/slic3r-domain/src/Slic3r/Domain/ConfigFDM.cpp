#include "Slic3r/Domain/ConfigFDM.hpp"


// Implementation of FDM configs is done in this file.


void init_fn(ConfigDefinitions& defs);

// Define the static object holding all definitions. Provide list of acceptable
// boxes and the init function.
ConfigDefinitions s_defs_fdm({"printer_settings", "filament_settings", "print_settings",
    "toolprint_settings", "object_settings", "volume_settings"}, init_fn);


// Now define the init function. This function will be called by ConfigDefinitions
// constructor and will fill the definitions with all the necessary data.
void init_fn(ConfigDefinitions& defs)
{
    using ConfigItemType::Bool;
    using ConfigItemType::Int;
    using ConfigItemType::Double;
    using ConfigItemType::String;
    using ConfigItemType::Bools;
    using ConfigItemType::Ints;
    using ConfigItemType::Doubles;
    using ConfigItemType::Strings;

    ConfigItemDef* def = nullptr;


    def = defs.add("extruder_num", Int);
    def->belongs_to = {"printer_settings"};
    def->init_fn = [](ConfigItem& item) -> void { item.set<int>(2); };

    def = defs.add("nozzle_high_flow", Bools);
    def->belongs_to = { "printer_settings"};
    def->init_fn = [](ConfigItem& item) { item.vec<bool>() = { true, false }; };

    def = defs.add("retract_length", Double);
    def->belongs_to = { "printer_settings"};
    def->belongs_to_optional = { "filament_settings"};
    def->init_fn_ex = [](ConfigItem& item, std::string_view box) {
        item.set<double>(box == "printer_settings" ? 0.7 : 0.5);
        };

    def = defs.add("travel_lift_before_obstacles", Bool);
    def->belongs_to = { "printer_settings" };
    def->belongs_to_optional = { "filament_settings", "object_settings" };
    def->init_fn = [](ConfigItem& item) { item.set(false); };

    def = defs.add("temperature", Int);
    def->belongs_to = { "filament_settings" };
    def->init_fn = [](ConfigItem& item) { item.set(230); };

    def = defs.add("speed", Int);
    def->belongs_to = { "toolprint_settings" };
    def->init_fn = [](ConfigItem& item) { item.set(230); };

    def = defs.add("output_filename_format", String);
    def->belongs_to = { "print_settings" };
    def->init_fn = [](ConfigItem& item) { item.set("{input_filename_base}.gcode"); };

    def = defs.add("gcode_flavor", ConfigItemType::Enum);
    def->belongs_to = { "print_settings" };
    def->enum_type = MyEnum{};
    def->enum_values = { { int(MyEnum::One), "one", "One" },
                         { int(MyEnum::Two), "two", "Two" } };
    def->init_fn = [](ConfigItem& item) { item.set(MyEnum::One); };
}



