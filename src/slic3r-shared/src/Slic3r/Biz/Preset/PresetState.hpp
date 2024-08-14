#pragma once

#include "libslic3r/Preset.hpp"

namespace Slic3r::Biz::Preset {

struct PresetState
{
    Slic3r::Preset& selected_preset;
    Slic3r::Preset edited_preset;
};

}
