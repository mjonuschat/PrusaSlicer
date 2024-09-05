#pragma once

#include <vector>

namespace Slic3r {
class Preset;
}

namespace Slic3r::Biz::Preset {

struct PresetRuntime
{
    const bool is_compatible;
    const Slic3r::Preset* preset;
};

struct PresetBundleRuntime
{
    using PresetRuntimeList = std::vector<PresetRuntime>;
    using PresetRuntimeListList = std::vector<PresetRuntimeList>;

    PresetRuntimeList print;
    PresetRuntimeListList materials;
};

}
