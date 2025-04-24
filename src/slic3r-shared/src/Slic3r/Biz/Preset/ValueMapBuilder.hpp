
#pragma once

#include "Slic3r/Biz/Expr/Eval.hpp"
#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"

namespace Slic3r::Biz::Preset {

void append_value(Expr::ValueMap& values, const std::string& name, const Domain::Preset::PresetValue& v);

void append_value(Expr::ValueMap& values, const char* prefix, const Domain::Preset::HwModel& v);

void append_values(Expr::ValueMap& values, const char* prefix, const Domain::Preset::PresetValueMap& features);

void append_printer_values(Expr::ValueMap& values, const Domain::Preset::HwPrinterConfig& printer);

void append_print_values(Expr::ValueMap& values, const Domain::Preset::EvaluatedPreset& print_preset);

void append_tool_values(Expr::ValueMap& values, const Domain::Preset::HwToolConfig& tool);

}
