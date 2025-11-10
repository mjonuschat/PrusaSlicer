#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <variant>
#include <vector>

#include "Slic3r/Domain/PrinterTechnology.hpp"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Domain/Percentage.hpp"
#include "Slic3r/Domain/JsonValue.hpp"

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

using FeatureValue = JsonValue;
using FeatureValueMap = std::map<std::string, FeatureValue>;

using Bools = std::vector<bool>;
using Strings = std::vector<std::string>;
using Doubles = std::vector<double>;
using Ints = std::vector<int>;
using OptInts = std::vector<std::optional<int>>;
using Percentages = std::vector<Percentage>;

using PresetValue = std::variant<
    std::monostate,
    Bools, Doubles, Ints, OptInts, Percentages, Vec2ds, Strings,
    bool, double, int, Percentage, Vec2d, std::string
>;
using PresetValueMap = std::map<std::string, PresetValue>;

}
