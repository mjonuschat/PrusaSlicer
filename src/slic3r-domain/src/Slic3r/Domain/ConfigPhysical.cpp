#include "Slic3r/Domain/ConfigPhysical.hpp"
#include "Slic3r/Domain/ConfigDefUtils.hpp"


namespace Slic3r::Domain {

std::string print_host_type_to_string(PrintHostType type) {
    switch (type) {
        case PrusaLink:         return "PrusaLink";
        case SL1Host:           return "SL1Host";
        case OctoPrint:         return "OctoPrint";
        case Moonraker:         return "Moonraker";
        case Duet:              return "Duet";
        case FlashAir:          return "FlashAir";
        case AstroBox:          return "AstroBox";
        case Repetier:          return "Repetier";
        case MKS:               return "MKS";
        default:                return "Unknown";
    }
}

// Implementation of physical printer configs is done in this file.

// Define our own marking functions, the regular ones are not accessible in Domain.
static const std::string& L(const std::string& s) { return s; }

void physical_printer_init_fn(ConfigDefinitions& defs);

// Define the static object holding all definitions. Provide list of acceptable
// boxes and the init function.
const ConfigDefinitions& get_defs_physical_printer() {
    static ConfigDefinitions s_defs_physical({PhysicalPrinterLocation{}}, physical_printer_init_fn);
    return s_defs_physical;
}

// Now define the init function. This function will be called by ConfigDefinitions
// constructor and will fill the definitions with all the necessary data.
void physical_printer_init_fn(ConfigDefinitions& defs)
{
    ConfigItemDef* def = nullptr;
    using Physical = PhysicalPrinterLocation;

    def = defs.add("physical_printer_user_given_name", typeid(std::string));
    def->location = Physical{};
    def->label = L("Name");
    def->tooltip = L("User given name to distinguish configurations.");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with("");
    def->gui_type = ConfigItemDef::GUIType::textfield;
    def->order = 0;

    def = defs.add("physical_printer_host", typeid(std::string));
    def->location = Physical{};
    def->label = L("Hostname, IP or URL");
    def->tooltip = L("Slic3r can upload G-code files to a printer host. This field should contain "
                   "the hostname, IP address or URL of the printer host instance. "
                   "Print host behind HAProxy with basic auth enabled can be accessed by putting the user name and password into the URL "
                   "in the following format: https://username:password@your-octopi-address/");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with("");
    def->gui_type = ConfigItemDef::GUIType::textfield;
    def->order = 1;

    def = defs.add("physical_printer_host_type", typeid(EnumWrapper));
    def->location = Physical{};
    def->label = L("Host Type");
    def->tooltip = L("Slic3r can upload G-code files to a printer host. This field must contain "
                   "the kind of the host.");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with(
        PrintHostType::PrusaLink,
        {{int(PrintHostType::PrusaLink), "prusalink", "PrusaLink"},
         {int(PrintHostType::SL1Host), "sl1", "SL1"},
         {int(PrintHostType::OctoPrint), "octoprint", "OctoPrint"},
         {int(PrintHostType::Moonraker), "moonraker", "Moonraker"},
         {int(PrintHostType::Duet), "duet", "Duet"},
         {int(PrintHostType::FlashAir), "flashair", "FlashAir"},
         {int(PrintHostType::AstroBox), "astrobox", "AstroBox"},
         {int(PrintHostType::Repetier), "repetier", "Repetier"},
         {int(PrintHostType::MKS), "mks", "MKS"}}
    );
    def->gui_type = ConfigItemDef::GUIType::combobox;
    def->order = 2;

    def = defs.add("physical_printer_authorization_type", typeid(EnumWrapper));
    def->location = Physical{};
    def->label = L("Authorization Type");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with(
        PrintHostAuthType::ApiKey,
        {{int(PrintHostAuthType::None), "none", L("None")},
         {int(PrintHostAuthType::ApiKey), "key", L("API key")},
         {int(PrintHostAuthType::Digest), "user", L("HTTP digest")}}
    );
    def->gui_type = ConfigItemDef::GUIType::combobox;
    def->order = 3;

    def = defs.add("physical_printer_api_key", typeid(std::string));
    def->location = Physical{};
    def->label = L("API Key / Password");
    def->tooltip = L("Slic3r can upload G-code files to a printer host. This field should contain "
                   "the API Key or the password required for authentication.");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with("");
    def->gui_type = ConfigItemDef::GUIType::textfield;
    def->order = 4;

    def = defs.add("physical_printer_user", typeid(std::string));
    def->location = Physical{};
    def->label = L("User");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with("");
    def->gui_type = ConfigItemDef::GUIType::textfield;
    def->order = 5;

    def = defs.add("physical_printer_password", typeid(std::string));
    def->location = Physical{};
    def->label = L("Password");
    def->gui_type = ConfigItemDef::GUIType::password;
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with("");
    def->gui_type = ConfigItemDef::GUIType::textfield;
    def->order = 6;

    def = defs.add("physical_printer_port", typeid(std::string));
    def->location = Physical{};
    def->label = L("Port");
    def->tooltip = L("Port number. Optional parameter.");
    def->gui_type = ConfigItemDef::GUIType::select_close;
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with("");
    def->gui_type = ConfigItemDef::GUIType::textfield;
    def->order = 7;

    def = defs.add("physical_printer_ca_file", typeid(std::string));
    def->location = Physical{};
    def->label = L("HTTPS CA File");
    def->tooltip = L("Custom CA certificate file can be specified for HTTPS OctoPrint connections, in crt/pem format. "
                   "If left blank, the default OS CA certificate repository is used.");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with("");
    def->gui_type = ConfigItemDef::GUIType::textfield;
    def->order = 8;

    // Only available on Windows.
    def = defs.add("physical_printer_ssl_ignore_revoke", typeid(bool));
    def->location = Physical{};
    def->label = L("Ignore HTTPS certificate revocation checks");
    def->tooltip = L("Ignore HTTPS certificate revocation checks in case of missing or offline distribution points. "
                     "One may want to enable this option for self signed certificates if connection fails.");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::PhysicalPrinter_General;
    def->init_fn = init_with(false);
    def->gui_type = ConfigItemDef::GUIType::checkbox;
    def->order = 9;

    /*
    def = defs.add("preset_names", typeid(std::vector<std::string>));
    def->location = Physical{};
    def->label = L("Printer preset names");
    def->tooltip = L("Names of presets related to the physical printer");
    def->category = ConfigItemDef::Category::Hidden;
    def->init_fn = init_with(std::vector<std::string>{});
    */

    def = defs.add("physical_printer_uuid", typeid(std::string));
    def->location = Physical{};
    def->label = L("UUID");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::Hidden;
    def->init_fn = init_with("");

    def = defs.add("physical_printer_preset_model", typeid(std::string));
    def->location = Physical{};
    def->label = L("UUID");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::Hidden;
    def->init_fn = init_with("");

    def = defs.add("physical_printer_preset_base_model", typeid(std::string));
    def->location = Physical{};
    def->label = L("UUID");
    def->cli = ConfigItemDef::nocli;
    def->category = ConfigItemDef::Category::Hidden;
    def->init_fn = init_with("");
}

} // namespace Slic3r::Domain
