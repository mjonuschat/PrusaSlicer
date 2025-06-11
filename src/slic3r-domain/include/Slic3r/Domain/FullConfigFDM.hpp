#pragma once

#include "Slic3r/Domain/ConfigBoxesFDM.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"

namespace Slic3r::Domain {


BoxOrBoxesVector as_boxes(const ConfigPackFDM& config_pack);

class FullConfigFDM : public FullConfig
{
public:
    FullConfigFDM(const ConfigPackFDM& config_pack);

    std::size_t tools_count() const {
        return m_tools_count;
    }

    std::size_t filaments_count() const {
        return m_filaments_count;
    }

    static FullConfigFDM defaults() {
        return {ConfigPackFDM{}};
    }

private:
    std::size_t m_tools_count;
    std::size_t m_filaments_count;
};

class PartialObjectConfigFDM : public PartialConfig {
public:
    PartialObjectConfigFDM(
        const ObjectSettings& object_settings,
        const std::size_t tools_count,
        const std::size_t filaments_count
    );
};

class PartialVolumeConfigFDM : public PartialConfig {
public:
    PartialVolumeConfigFDM(
        const VolumeSettings& volume_settings,
        const std::size_t tools_count,
        const std::size_t filaments_count
    );
};

using FullConfigFDMPtr = std::shared_ptr<const FullConfigFDM>;
using PartialObjectConfigFDMPtr = std::shared_ptr<const PartialObjectConfigFDM>;
using PartialVolumeConfigFDMPtr = std::shared_ptr<const PartialVolumeConfigFDM>;

} // namespace Slic3r::Domain
