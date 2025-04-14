#pragma once

#include "Slic3r/Domain/Config.hpp"


// This is an example of using Config infrastructure.

// First, define a static object of ConfigDefinitions. This object will hold all
// the definitions of the config items.
extern ConfigDefinitions s_defs_physical;



// Next, define all enums that should be used in the config.
enum class AuthorizationType {
    KeyPassword, UserPassword
};
enum PrintHostType {
   htPrusaLink, htPrusaConnect, htOctoPrint, htMoonraker, htDuet, htFlashAir, htAstroBox, htRepetier, htMKS, htPrusaConnectNew
};

// Then, define all types of ConfigBoxes that will be used. Provide our list
// of definitions and the type of the box (which must match definitions).

class PhysicalPrinterSettings : public ConfigBox
{
public:
    PhysicalPrinterSettings() : ConfigBox(s_defs_physical, "physical_printer_settings") {}
};
