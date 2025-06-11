#pragma once

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigBoxesSLA.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"


namespace Slic3r::Domain {

BoxOrBoxesVector as_boxes(const ConfigPackSLA& config_pack);

class FullConfigSLA : public FullConfig
{
public:
    FullConfigSLA(const ConfigPackSLA& config_pack);

    static FullConfigSLA defaults() {
        return {ConfigPackSLA{}};
    }
};

class PartialObjectConfigSLA : public PartialConfig {
public:
    PartialObjectConfigSLA(
        const SLAObjectSettings& object_settings
    );
};

using FullConfigSLAPtr = std::shared_ptr<const FullConfigSLA>;
using PartialObjectConfigSLAPtr = std::shared_ptr<const PartialObjectConfigSLA>;

} // namespace Slic3r::Domain
