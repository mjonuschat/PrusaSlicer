#include "Slic3r/Domain/Preset/Bundle.hpp"

namespace Slic3r::Domain::Preset {

const EvaluatedPrinterPreset* Bundle::find_printer_preset_by_id(const std::string& id) const
{
    for (const auto& [_, v] : evaluated_presets) {
        auto it = std::ranges::find_if(v, [&id](const EvaluatedPrinterPreset& p) {
            return p.preset.id == id;
        });
        if (it != v.end())
            return &*it;
    }
    return nullptr;
}

} // namespace Slic3r::Domain::Preset
