#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <variant>
#include <vector>

#include "Slic3r/Domain/PrinterTechnology.hpp"

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

template <typename E, typename T>
using EnumCollection = std::map<E, std::vector<T>>;

using PresetValue = std::variant<bool, float, std::string>;
using PresetValueMap = std::map<std::string, PresetValue>;

template <typename ValueT>
void override_values(PresetValueMap& dest, const std::map<std::string, ValueT>& overrides)
{
    for (const auto& [key, value] : overrides)
        dest[key] = value;
}


}
