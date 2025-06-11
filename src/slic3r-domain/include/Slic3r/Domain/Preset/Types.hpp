#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <variant>
#include <vector>

#include "Slic3r/Domain/PrinterTechnology.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Domain::Preset {

enum class PresetKind : uint8_t
{
    FdmPrinter = 0,
    FdmPrint,
    FdmToolPrint,
    FdmMaterial,
    SlaPrinter = 8,
    SlaPrint,
    SlaToolPrint,
    SlaMaterial
};

inline PresetKind printer_kind(PrinterTechnology technology)
{ return technology == PrinterTechnology::FFF ? PresetKind::FdmPrinter : PresetKind::SlaPrinter; }

inline PresetKind print_kind(PrinterTechnology technology)
{ return technology == PrinterTechnology::FFF ? PresetKind::FdmPrint : PresetKind::SlaPrint; }

inline PresetKind tool_print_kind(PrinterTechnology technology)
{ return technology == PrinterTechnology::FFF ? PresetKind::FdmToolPrint : PresetKind::SlaToolPrint; }

inline PresetKind material_kind(PrinterTechnology technology)
{ return technology == PrinterTechnology::FFF ? PresetKind::FdmMaterial : PresetKind::SlaMaterial; }

using FeatureValue = std::variant<bool, float, std::string>;
using FeatureValueMap = std::map<std::string, FeatureValue>;

using Bools = std::vector<bool>;
using Strings = std::vector<std::string>;
using Floats = std::vector<float>;

using PresetValue = std::variant<
    std::monostate,
    Bools, Floats, Vec2ds, Strings,
    bool, float, Vec2d, std::string
>;
using PresetValueMap = std::map<std::string, PresetValue>;

template <typename ValueT>
void override_values(PresetValueMap& dest, const std::map<std::string, ValueT>& overrides)
{
    for (const auto& [key, value] : overrides)
        dest[key] = value;
}

template <typename ValueT>
void override_values(FeatureValueMap& dest, const std::map<std::string, ValueT>& overrides)
{
    for (const auto& [key, value] : overrides)
        dest[key] = value;
}



}
