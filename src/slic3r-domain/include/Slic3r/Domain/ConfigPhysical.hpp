#pragma once

#include "Slic3r/Domain/Config.hpp"


namespace Slic3r::Domain {

// This is an example of using Config infrastructure.

// First, define a static object of ConfigDefinitions. This object will hold all
// the definitions of the config items.
extern ConfigDefinitions s_defs_physical;

// Next, define all enums that should be used in the config.
enum class PrintHostAuthType {
    None,
    ApiKey,
    Digest
};
enum PrintHostType {
    Local,
    PrusaLink,
    PrusaLinkStorage,
    PrusaConnect,
    SL1Host,
    OctoPrint,
    Moonraker,
    Duet,
    FlashAir,
    AstroBox,
    Repetier,
    MKS
};

// Then, define all types of ConfigBoxes that will be used. Provide our list
// of definitions and the type of the box (which must match definitions).

class PhysicalPrinterSettings : public ConfigBox
{
public:
    PhysicalPrinterSettings() : ConfigBox(s_defs_physical, "physical_printer_settings") {}
};

} // namespace Slic3r::Domain
