#pragma once

#include <vector>

#include "PresetRuntime.hpp"

namespace Slic3r::Biz::Preset {

struct PresetBundleRuntime
{
    std::vector<PresetRuntime>                  print;
    std::vector<std::vector<PresetRuntime>>     materials;

};

}
