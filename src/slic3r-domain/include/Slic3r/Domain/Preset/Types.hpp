#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <variant>
#include <vector>

namespace Slic3r::Domain::Preset {

enum class PresetKind : uint8_t
{
    FdmPrinter = 0,
    FdmPrint,
    FdmToolPrint,
    FdmMaterial
};

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
