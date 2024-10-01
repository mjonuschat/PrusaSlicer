#pragma once

#include "libslic3r/Preset.hpp"

namespace Slic3r::Biz::Preset {

struct PresetRuntime
{
    Slic3r::Preset*     preset          { nullptr };
    bool                is_compatible   { true };
};

}
