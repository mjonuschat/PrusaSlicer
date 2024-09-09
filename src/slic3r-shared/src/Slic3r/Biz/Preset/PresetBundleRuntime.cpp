#include "PresetBundleRuntime.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"


namespace Slic3r::Biz::Preset {


void PresetBundleRuntime::update_compatible_prints(const PresetBundle& bundle, const Slic3r::Preset& active_printer)
{
    print.clear();
    // TODO: fill in compatible prints
    // see PresetBundle::update_compatible(PresetSelectCompatibleType select_other_print_if_incompatible, PresetSelectCompatibleType select_other_filament_if_incompatible)

}

void PresetBundleRuntime::update_compatible_materials(const PresetBundle& bundle, const Slic3r::Preset& active_printer, const Slic3r::Preset& active_print)
{
    for (auto& mat : materials)
        mat.clear();
    // TODO: fill in compatible print materials/filaments
    // see PresetBundle::update_filaments_compatible(PresetSelectCompatibleType select_other_filament_if_incompatible, int extruder_idx/* = -1*/)
}



}
