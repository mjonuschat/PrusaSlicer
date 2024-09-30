#include "Slic3r/Biz/Preset/PresetBundleRuntime.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"


namespace Slic3r::Biz::Preset {


void PresetBundleRuntime::update_compatible_prints(const PresetBundle& bundle, const Slic3r::Preset& active_printer)
{
    print.clear();
    const auto& prints = active_printer.printer_technology() == ptFFF ? bundle.prints : bundle.sla_prints;
    for (auto it = prints.cbegin(); it != prints.cend(); ++it) {
        this->print.push_back({true, &*it});
    }
    // TODO: fill in compatible prints
    // see PresetBundle::update_compatible(PresetSelectCompatibleType select_other_print_if_incompatible, PresetSelectCompatibleType select_other_filament_if_incompatible)

}

void PresetBundleRuntime::update_compatible_materials(const PresetBundle& bundle, const Slic3r::Preset& active_printer, const Slic3r::Preset& active_print)
{
    for (auto& mat : materials)
        mat.clear();

    size_t num_extruders = active_printer.printer_technology() == ptFFF
        ? active_printer.config.option<ConfigOptionFloats>("nozzle_diameter"  )->values.size()
        : 1;

    const auto& mats = active_printer.printer_technology() == ptFFF ?  bundle.filaments : bundle.sla_materials;
    for (size_t extruder_idx = 0; extruder_idx < num_extruders; extruder_idx++ ) {
        PresetRuntimeList dest_materials;
        for (const auto & mat : mats) {
            dest_materials.push_back({true, &mat});
        }
        materials.push_back(std::move(dest_materials));
    }
    // TODO: fill in compatible print materials/filaments
    // see PresetBundle::update_filaments_compatible(PresetSelectCompatibleType select_other_filament_if_incompatible, int extruder_idx/* = -1*/)
}



}
