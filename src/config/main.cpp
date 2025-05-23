#include "Slic3r/Domain/ConfigFDM.hpp"
#include "Slic3r/Domain/ConfigSLA.hpp"
#include "Slic3r/Domain/ConfigPhysical.hpp"
#include "Slic3r/Biz/Config/ConfigSerialize.hpp"
#include "Slic3r/Biz/Config/ConfigLegacy.hpp"

#include "libslic3r/Model.hpp"
#include "Slic3r/Biz/Config/3mf_legacy.hpp"



#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

using Slic3r::Domain::ConfigPack;
using Slic3r::Domain::ConfigPackFDM;
using Slic3r::Domain::ConfigPackSLA;

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

    {
        Slic3r::Model model;
        ConfigPack cfg;
        boost::optional<Slic3r::Semver> prusaslicer_generator_version;
        Slic3rLegacy::WipeTowersOnBeds wipe_towers;
        Slic3rLegacy::CustomGCodesOnBeds custom_gcodes;
        Slic3rLegacy::load_3mf_legacy("test.3mf", cfg, &model, true, prusaslicer_generator_version, wipe_towers, custom_gcodes);

        Slic3rLegacy::store_3mf_legacy("out.3mf", &model, std::optional<ConfigPack>(cfg), false, wipe_towers, custom_gcodes);
        return 0;
    }



    {
        SLAPrinterSettings printer_s;
        SLAPrintSettings print_s;
        SLAMaterialSettings material_s;

        ConfigPack cfg = load_config_from_legacy_file("test_sla.ini");
        if (std::holds_alternative<ConfigPackSLA>(cfg)) {
            const auto& sla = std::get<ConfigPackSLA>(cfg);
            printer_s = sla.sla_printer_settings;
            material_s = sla.sla_material_settings;
            print_s = sla.sla_print_settings;            
        } else {
            PANIC();
        }   

        std::vector<
		std::variant<
		    std::reference_wrapper<const ConfigBox>,
	        std::vector<std::reference_wrapper<const ConfigBox>>
	    >
	    > list;
        list.emplace_back(printer_s);
        list.emplace_back(print_s);
        list.emplace_back(material_s);

        std::cout << serialize(list, 2, false);
        std::ofstream leg("test_sla-roundtrip.ini");
        leg << serialize_as_legacy_config(std::get<ConfigPackSLA>(cfg)) << std::endl;
    }




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

    ConfigPack cfg = load_config_from_legacy_file("test.ini");
    if (std::holds_alternative<ConfigPackFDM>(cfg)) {
        const auto& fdm = std::get<ConfigPackFDM>(cfg);
        printer_s = fdm.printer;
        fs1 = fdm.filament[0];
        fs2 = fdm.filament[1];
        ps1 = fdm.print;
        tps1 = fdm.tool[0];
        tps2 = fdm.tool[1];
    } else {
        PANIC();
    }   

    std::vector<
		std::variant<
		    std::reference_wrapper<const ConfigBox>,
	        std::vector<std::reference_wrapper<const ConfigBox>>
	    >
	> list;
    list.emplace_back(printer_s);
    list.emplace_back(ps1);
    list.emplace_back(std::vector<std::reference_wrapper<const ConfigBox>>{tps1, tps2});
    list.emplace_back(std::vector<std::reference_wrapper<const ConfigBox>>{fs1, fs2});

    std::cout << serialize(list, 2, false);

    std::ofstream leg("test-roundtrip.gcode");
    leg << serialize_as_legacy_config(std::get<ConfigPackFDM>(cfg)) << std::endl;


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
