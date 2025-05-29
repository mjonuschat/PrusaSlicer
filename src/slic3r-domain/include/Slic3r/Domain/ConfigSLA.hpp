#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigCommon.hpp"


namespace Slic3r::Domain {

// First, define a static object of ConfigDefinitions. This object will hold all
// the definitions of the config items.
extern ConfigDefinitions s_defs_sla;



// Next, define all enums that should be used in the config.
enum class SLADisplayOrientation {
    sladoLandscape,
    sladoPortrait
};

enum SLAMaterialSpeed { slamsSlow, slamsFast, slamsHighViscosity };

namespace sla {
    enum class SupportTreeType { Default, Branching, Organic };
    enum class PillarConnectionMode { zigzag, cross, dynamic };
}

enum TowerSpeeds : int {
    tsLayer1,
    tsLayer2,
    tsLayer3,
    tsLayer4,
    tsLayer5,
    tsLayer8,
    tsLayer11,
    tsLayer14,
    tsLayer18, 
    tsLayer22,
    tsLayer24,
};

enum TiltSpeeds : int {
    tsMove120,
    tsLayer200,
    tsMove300,
    tsLayer400,
    tsLayer600,
    tsLayer800,
    tsLayer1000,
    tsLayer1250,
    tsLayer1500,
    tsLayer1750,
    tsLayer2000,
    tsLayer2250,
    tsMove5120,
    tsMove8000,
};




// Then, define all types of ConfigBoxes that will be used. Provide our list
// of definitions and the type of the box (which must match definitions).

class SLAPrintSettings : public ConfigBox
{
public:
    SLAPrintSettings() : ConfigBox(s_defs_sla, "sla_print_settings") {}
};
class SLAMaterialSettings : public ConfigBox
{
public:
    SLAMaterialSettings() : ConfigBox(s_defs_sla, "sla_material_settings") {}
};
class SLAPrinterSettings : public ConfigBox
{
public:
    SLAPrinterSettings() : ConfigBox(s_defs_sla, "sla_printer_settings") {}
};
class SLAObjectSettings : public ConfigBox
{
public:
    SLAObjectSettings() : ConfigBox(s_defs_sla, "sla_object_settings") {}
};


class FullConfigSLA : public FullConfig
{
public:
    FullConfigSLA(const SLAPrinterSettings& printer_s,
                  const SLAPrintSettings& print_s,
                  const SLAMaterialSettings& material_s);

    std::string_view name() const override { return "SLA"; }

    static FullConfigSLA defaults() {
        return FullConfigSLA{
            SLAPrinterSettings{},
            SLAPrintSettings{},
            SLAMaterialSettings{}
        };
    }
};

using FullConfigSLAPtr = std::shared_ptr<const FullConfigSLA>;
using SLAObjectSettingsPtr = std::shared_ptr<const SLAObjectSettings>;

} // namespace Slic3r::Domain
