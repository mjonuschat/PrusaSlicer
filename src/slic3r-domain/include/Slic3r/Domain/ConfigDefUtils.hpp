#pragma once


#include "Slic3r/Domain/ConfigValue.hpp"
#include <vector>

namespace Slic3r::Domain {
std::vector<EnumValueDefsPtr>& get_enum_defs();

class ConfigDefinitions;

// Reads the existing choices back out of the option's init_fn rather than
// having the caller restate them, which would go stale the moment upstream adds
// a choice of its own.
void append_enum_choice(
    ConfigDefinitions& defs,
    const std::string_view name,
    int enum_value,
    std::string str_serialized,
    std::string str_ui
);

template <typename T>
requires std::is_enum_v<typename T::value_type>
auto init_with(const T& values, const EnumValueDefs* def) {
    const EnumVectorWrapper enum_wrappers{values, def};
    return [enum_wrappers](){return ConfigValue{enum_wrappers};};
}


template <typename T>
requires std::is_enum_v<T>
static auto init_with(const T& value, const EnumValueDefs* def) {
    return [value, def](){return ConfigValue{EnumWrapper{value, def}};};
}

template <typename T>
requires std::is_enum_v<typename T::value_type>
auto init_with(const T& values, const EnumValueDefs& enum_values) {
    get_enum_defs().push_back(std::make_unique<EnumValueDefs>(enum_values));
    const EnumVectorWrapper enum_wrappers{values, get_enum_defs().back().get()};

    return [enum_wrappers](){return ConfigValue{enum_wrappers};};
}

template <typename T>
requires std::is_enum_v<T>
auto init_with(const T& value, const EnumValueDefs& enum_values) {
    get_enum_defs().push_back(std::make_unique<EnumValueDefs>(enum_values));
    return [value, enum_def{get_enum_defs().back().get()}](){return ConfigValue{EnumWrapper{value, enum_def}};};
}

template <typename T>
requires (!std::is_enum_v<T>)
auto init_with(const T& value) {
    return [value](){return ConfigValue{value};};
}
}
