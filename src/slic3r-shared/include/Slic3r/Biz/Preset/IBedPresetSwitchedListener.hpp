#pragma once

#include "libslic3r/Preset.hpp"

namespace Slic3r::Biz::Preset {

class IBedPresetSwitchedListener {
public:
    virtual ~IBedPresetSwitchedListener() = default;

    virtual void on_bed_preset_switched(Slic3r::Preset::Type preset_type, size_t opt_extruder_idx = 0) = 0;
};

}
