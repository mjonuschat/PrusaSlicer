#pragma once

#include "PresetState.hpp"

namespace Slic3r::Biz::Preset {

struct PresetInteractorConfigContainerContext
{
    size_t config_container_id;
    PresetState printer;
    PresetState print;
    PresetState material;
};

}
