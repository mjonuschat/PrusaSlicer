#include "Slic3r/Domain/ConfigFDM.hpp"
#include "Slic3r/Domain/ConfigPhysical.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/Biz/Config/ConfigLegacy.hpp"

#include <iostream>
#include <vector>


static void print(const std::variant<std::string, std::vector<std::string>>& in)
{
    if (std::holds_alternative<std::string>(in)) {
        std::cout << std::get<std::string>(in) << std::endl;
    }
    else {
        const auto& vec = std::get<std::vector<std::string>>(in);
        std::cout << std::endl;
        for (const auto& str : vec) {
            std::cout << "  - " << str << std::endl;
        }
    }
}


int main(int, char* [])
{
    using namespace Slic3r::Domain;
    using namespace Slic3r::Biz;

    PrinterSettings printer_s;
    FilamentSettings fs1;
    FilamentSettings fs2;
    PrintSettings ps1;
    PrintSettings ps2;
    ToolPrintSettings tps1;
    ToolPrintSettings tps2;
    ObjectSettings os;
    VolumeSettings vs;

    PhysicalPrinterSettings pps;

    //os.opt("travel_lift_before_obstacles").set_null(false);
    //os.opt("travel_lift_before_obstacles").set<bool>(true);
    //ps.opt("gcode_flavor").set_enum_from_string("two");

    //load_from_legacy_file("test.ini", ps);

    ps2.opt("gcode_comments").set(true);


    FullConfigFDM fullc1(printer_s, {fs1}, ps1, {tps1});
    FullConfigFDM fullc2(printer_s, {fs2}, ps2, {tps2});

    ConfigView cv(fullc1, os);
    int a = cv.get<int>("perimeters");

    std::vector<std::string> diff = fullc1.diff_keys(fullc2);

    std::cout << std::endl;
    
    //std::cout << "PRINTER SETTINGS\n" << serialize(printer_s).dump(4) << std::endl << "=============================" << std::endl;
    //std::cout << "FILAMENT SETTINGS\n" << serialize(fs).dump(4) << std::endl << "=============================" << std::endl;
    //std::cout << "PRINT SETTINGS\n" << serialize(ps).dump(4) << std::endl << "=============================" << std::endl;
    //std::cout << "TOOL PRINT SETTINGS\n" << serialize(tps).dump(4) << std::endl << "=============================\n\n\n" << std::endl;
    //std::cout << "OBJECT SETTINGS\n" << serialize(os).dump(4) << std::endl << "=============================\n\n\n" << std::endl;
    //std::cout << "PHYSICAL PRINTER SETTINGS\n" << serialize(pps).dump(4) << std::endl << "=============================\n\n\n" << std::endl;

    //for (const ConfigItemDef& def : s_defs_fdm.defs()) {
    //    bool tool_dependent = false;
    //    for (const auto& list : { &def.belongs_to, &def.belongs_to_optional })
    //        tool_dependent |= std::any_of(list->begin(), list->end(),
    //            [](const auto& t) { return t == "toolprint_settings" || t == "filament_settings"; });

    //    std::cout << def.name << ": ";
    //    //if (tool_dependent)
    //    //    print(serialize_to_string(fullc.opt(def.name, 0)));
    //    //else
    //    //    print(serialize_to_string(fullc.opt(def.name)));
    //}



    return 0;
}






/*
* 
* // Proc?
 - prilis slozity retezec dedeni soucasnych ConfigBase atd. a s tim souvisejici nemoznost s tim pruzne pracovat
 - odstineni od implementacnich detailu (dynamic_cast atd)
 - prilisna obecnost DynamicPrintConfigu (nevim co jsem dostal, nevim jestli je to komplet)
 - kompilacni casy (skoro kazdy soubor includuje obrovskou hromadu defines a templatu)
 - home-brew serializace a deserializace



Odkud načítáme profily:
- gcode
- bgcode
- json
- 3mf


TODOS:

- knihovna MagicEnum?
- validační funkce?
- vynucování fixních délek vektorů (nebo mod N)? Point / Points.

*/