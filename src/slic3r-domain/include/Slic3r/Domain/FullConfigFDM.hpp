#pragma once

#include "Slic3r/Domain/ConfigBoxesFDM.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Uuid.hpp"

namespace Slic3r::Domain {


BoxOrBoxesVector as_boxes(const ConfigPackFDM& config_pack);
MutBoxOrBoxesVector as_mut_boxes(ConfigPackFDM& config_pack);

class FullConfigFDM : public FullConfig
{
public:
    FullConfigFDM(
        const ConfigPackFDM& config_pack,
        const std::vector<unsigned>& extruder_candidates,
        const Preset::HwPrinterConfig& hw_config
    );

    const Preset::HwPrinterConfig& hw_config() const {
        return m_hw_config;
    }

    static FullConfigFDM defaults() {

        Preset::HwToolConfig tool_config;

        Preset::HwPrinterConfig hw_config{
            .id                   = generate_uuid(),
            .printer_id           = {},
            .legacy_printer_model = {},
            .vendor_id            = {},
            .repo_id              = {},
            .repo_version         = {},
            .name                 = {},
            .short_name           = {},
            .technology           = PrinterTechnology::FFF,
            .model                = {},
            .tool_count           = 1,
            .features             = {},
            .visual               = {},
            .tools                = {1, Preset::HwToolConfig{}},
            .feeders              = {
                {Preset::Address{0},
                    Preset::HwFeederConfig{
                        .id         = generate_uuid(),
                        .type       = Preset::FeederType::Manual,
                        .model      = {},
                        .slot_count = 1,
                        .features   = {}
                 }}
            },
            .materials = {},
            .sheet     = {}
        };

        return {ConfigPackFDM{}, {0}, hw_config};
    }

private:
    Preset::HwPrinterConfig m_hw_config;
};

class PartialObjectConfigFDM : public PartialConfig {
public:
    PartialObjectConfigFDM(
        const ObjectSettings& object_settings,
        const std::size_t material_slot_count
    );
};

class PartialVolumeConfigFDM : public PartialConfig {
public:
    PartialVolumeConfigFDM(
        const VolumeSettings& volume_settings,
        const std::size_t material_slot_count
    );
};

using FullConfigFDMPtr = std::shared_ptr<const FullConfigFDM>;
using PartialObjectConfigFDMPtr = std::shared_ptr<const PartialObjectConfigFDM>;
using PartialVolumeConfigFDMPtr = std::shared_ptr<const PartialVolumeConfigFDM>;

} // namespace Slic3r::Domain
