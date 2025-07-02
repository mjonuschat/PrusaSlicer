
#pragma once

#include "Slic3r/Biz/Expr/Eval.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"

namespace Slic3r::Biz::Preset {

template <typename ...Ts>
void append_value(Expr::ValueMap& values, const std::string& name, const std::variant<Ts...>& v)
{
    Expr::Value val;
    if (std::holds_alternative<std::string>(v)) {
        val = std::get<std::string>(v);
    } else if (std::holds_alternative<double>(v)) {
        val = std::get<double>(v);
    } else if (std::holds_alternative<bool>(v)) {
        val = std::get<bool>(v);
    } else {
        PANIC("unsupported type");
    }
    values[name] = val;
}

template <typename T>
concept StringConvertable = requires(T t)
{
    { std::to_string(t) } -> std::same_as<std::string>;
};

template <StringConvertable T>
void append_value(Expr::ValueMap& values, const std::string& name, const T& v)
{
    values[name] = std::to_string(v);
}


inline void append_value(Expr::ValueMap& values, const std::string& name, const std::string& v)
{
    values[name] = v;
}


void append_value(Expr::ValueMap& values, const char* prefix, const Domain::Preset::HwModel& v);

void append_values(Expr::ValueMap& values, const char* prefix, const Domain::Preset::FeatureValueMap& features);

void append_printer_values(Expr::ValueMap& values, const Domain::Preset::HwPrinterConfig& printer);

void append_print_values(Expr::ValueMap& values, const Domain::ConfigBox& print_preset);

void append_tool_values(Expr::ValueMap& values, const Domain::Preset::HwToolConfig& tool);

void append_sheet_values(Expr::ValueMap& values, const Domain::Preset::HwSheetConfig& sheet);

}
