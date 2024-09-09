#pragma once

#include <vector>

namespace Slic3r {
class Preset;
class PresetBundle;
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

    void update_compatible_prints(const PresetBundle& bundle, const Slic3r::Preset& active_printer);
    void update_compatible_materials(const PresetBundle& bundle, const Slic3r::Preset& active_printer, const Slic3r::Preset& active_print);
};

}
