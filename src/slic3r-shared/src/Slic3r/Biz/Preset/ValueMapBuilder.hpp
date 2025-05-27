
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
    } else if (std::holds_alternative<float>(v)) {
        val = std::get<float>(v);
    } else if (std::holds_alternative<bool>(v)) {
        val = std::get<bool>(v);
    } else {
        PANIC("unsupported type");
    }
    values[name] = val;

}


void append_value(Expr::ValueMap& values, const char* prefix, const Domain::Preset::HwModel& v);

void append_values(Expr::ValueMap& values, const char* prefix, const Domain::Preset::FeatureValueMap& features);

void append_printer_values(Expr::ValueMap& values, const Domain::Preset::HwPrinterConfig& printer);

void append_print_values(Expr::ValueMap& values, const Domain::Preset::EvaluatedPreset& print_preset);

void append_tool_values(Expr::ValueMap& values, const Domain::Preset::HwToolConfig& tool);

}
