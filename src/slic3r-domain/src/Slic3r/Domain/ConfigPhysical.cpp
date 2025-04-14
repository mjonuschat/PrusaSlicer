#include "Slic3r/Domain/ConfigPhysical.hpp"


// Implementation of physical printer configs is done in this file.

// Define our own marking functions, the regular ones are not accessible in Domain.
static const std::string& L(const std::string& s) { return s; }
static const std::string& L_CONTEXT(const std::string& s, const std::string& ctx) { return s; }

void physical_printer_init_fn(ConfigDefinitions& defs);

// Define the static object holding all definitions. Provide list of acceptable
// boxes and the init function.
ConfigDefinitions s_defs_physical({"physical_printer_settings"}, physical_printer_init_fn);

// JUST TEMPORARY UNTIL WE DECIDE WHAT TO DO WITH MODES.
// Right now, let's just define the constants so the defs compile.
enum { comSimple, comAdvanced, comExpert };

// Little helper to save some typing:
#define SET_DEFAULT(v) def->init_fn = [](ConfigItem& item) { item.set(v); };

// Now define the init function. This function will be called by ConfigDefinitions
// constructor and will fill the definitions with all the necessary data.
void physical_printer_init_fn(ConfigDefinitions& defs)
{
    using ConfigItemType::Bool;
    using ConfigItemType::Int;
    using ConfigItemType::Double;
    using ConfigItemType::String;
    using ConfigItemType::Enum;
    using ConfigItemType::Percent;
    using ConfigItemType::FloatOrPercent;
    using ConfigItemType::Bools;
    using ConfigItemType::Ints;
    using ConfigItemType::Doubles;
    using ConfigItemType::Strings;

    ConfigItemDef* def = nullptr;


    def = defs.add("host_type", Enum);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("Host Type");
    def->tooltip = L("Slic3r can upload G-code files to a printer host. This field must contain "
                   "the kind of the host.");
    def->enum_type = PrintHostType::htPrusaLink;
    def->enum_values = {
        { int(PrintHostType::htPrusaLink),    "prusalink",    "PrusaLink" },
        { int(PrintHostType::htPrusaConnect), "prusaconnect", "PrusaConnect" },
        { int(PrintHostType::htOctoPrint),    "octoprint",    "OctoPrint" },
        { int(PrintHostType::htMoonraker),    "moonraker",    "Klipper (via Moonraker)" },
        { int(PrintHostType::htDuet),         "duet",         "Duet" },
        { int(PrintHostType::htFlashAir),     "flashair",     "FlashAir" },
        { int(PrintHostType::htAstroBox),     "astrobox",     "AstroBox" },
        { int(PrintHostType::htRepetier),     "repetier",     "Repetier" },
        { int(PrintHostType::htMKS),          "mks",          "MKS" } };
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT(PrintHostType::htPrusaLink);
    
    def = defs.add("print_host", String);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("Hostname, IP or URL");
    def->tooltip = L("Slic3r can upload G-code files to a printer host. This field should contain "
                   "the hostname, IP address or URL of the printer host instance. "
                   "Print host behind HAProxy with basic auth enabled can be accessed by putting the user name and password into the URL "
                   "in the following format: https://username:password@your-octopi-address/");
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT("");

    def = defs.add("printhost_apikey", String);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("API Key / Password");
    def->tooltip = L("Slic3r can upload G-code files to a printer host. This field should contain "
                   "the API Key or the password required for authentication.");
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT("");
    
    def = defs.add("printhost_port", String);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("Printer");
    def->tooltip = L("Name of the printer");
    def->gui_type = ConfigItemDef::GUIType::select_close;
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT("");
    
    def = defs.add("printhost_cafile", String);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("HTTPS CA File");
    def->tooltip = L("Custom CA certificate file can be specified for HTTPS OctoPrint connections, in crt/pem format. "
                   "If left blank, the default OS CA certificate repository is used.");
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT("");

    def = defs.add("printhost_user", String);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("User");
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT("");
    
    def = defs.add("printhost_password", String);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("Password");
    def->gui_type = ConfigItemDef::GUIType::password;
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT("");

    // Only available on Windows.
    def = defs.add("printhost_ssl_ignore_revoke", Bool);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("Ignore HTTPS certificate revocation checks");
    def->tooltip = L("Ignore HTTPS certificate revocation checks in case of missing or offline distribution points. "
                     "One may want to enable this option for self signed certificates if connection fails.");
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT(false);
    
    def = defs.add("preset_names", Strings);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("Printer preset names");
    def->tooltip = L("Names of presets related to the physical printer");
    def->mode = comAdvanced;
    def->init_fn = [](ConfigItem& item) { item.vec<std::string>() = {}; };

    def = defs.add("printhost_authorization_type", Enum);
    def->belongs_to = { "physical_printer_settings" };
    def->label = L("Authorization Type");
    def->enum_type = AuthorizationType::KeyPassword;
    def->enum_values = { { int(AuthorizationType::KeyPassword), "key", L("API key") },
                         { int(AuthorizationType::UserPassword), "user", L("HTTP digest") } };
    def->mode = comAdvanced;
    def->cli = ConfigItemDef::nocli;
    SET_DEFAULT(AuthorizationType::KeyPassword);
}



