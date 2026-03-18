#pragma once

#include "Slic3r/Domain/Config.hpp"

#include <string>

namespace Slic3r::Domain {

// This is an example of using Config infrastructure.

// First, define a static object of ConfigDefinitions. This object will hold all
// the definitions of the config items.
const ConfigDefinitions& get_defs_physical_printer();

// Next, define all enums that should be used in the config.
enum class PrintHostAuthType {
    None,
    ApiKey,
    Digest
};
enum PrintHostType {
    PrusaLink,
    PrusaLinkStorage,
    SL1Host,
    OctoPrint,
    Moonraker,
    Duet,
    FlashAir,
    AstroBox,
    Repetier,
    MKS
};

std::string print_host_type_to_string(PrintHostType type);



// Then, define all types of ConfigBoxes that will be used. Provide our list
// of definitions and the type of the box (which must match definitions).

class PhysicalPrinterSettings : public ConfigBox
{
public:
    PhysicalPrinterSettings() : ConfigBox(get_defs_physical_printer(), PhysicalPrinterLocation{}) {}
};

} // namespace Slic3r::Domain
