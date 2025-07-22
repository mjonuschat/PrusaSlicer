#pragma once

#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"

namespace Slic3r::Domain::Preset {
struct SelectedPreset
{
    HwPrinterConfig hw_config;
    EvaluatedPrinterPreset::Preset printer;
    EvaluatedPrintPreset::Preset print;
    std::vector<EvaluatedToolPrintPreset::Preset> tools;
    std::vector<EvaluatedMaterialPreset::Preset> materials;

    PrinterTechnology technology() const
    {
        return hw_config.technology;
    }

    [[nodiscard]] std::string bed_model() const;
    [[nodiscard]] std::string bed_texture() const;

    ConfigPack config() const;
};

}
