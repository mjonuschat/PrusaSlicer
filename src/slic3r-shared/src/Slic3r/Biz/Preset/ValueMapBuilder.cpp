#include "Slic3r/Biz/Preset/ValueMapBuilder.hpp"

namespace Slic3r::Biz::Preset {

void append_value(Expr::ValueMap& values, const std::string& name, const Domain::Preset::PresetValue& v)
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

void append_value(Expr::ValueMap& values, const char* prefix, const Domain::Preset::HwModel& v)
{
    values[prefix + std::string("model")] = v.model;
    values[prefix + std::string("base_model")] = v.base_model;

}

void append_values(Expr::ValueMap& values, const char* prefix, const Domain::Preset::PresetValueMap& features)
{
    for (const auto& [k, v] : features)
        append_value(values, prefix + k, v);
}

void append_printer_values(Expr::ValueMap& values, const Domain::Preset::HwPrinterConfig& printer)
{
    append_value(values, "printer.", printer.model);
    append_values(values, "printer.", printer.features);

    if (printer.feeders.empty()) {
        append_value(values, "feeder.", Domain::Preset::HwModel{});
    } else {
        // At the moment only single feeder is supported
        ASSERT(printer.feeders.size() == 1);

        const auto& feeder = printer.feeders.begin()->second;
        append_value(values, "feeder.", feeder.model);
    }
}

void append_print_values(Expr::ValueMap& values, const Domain::Preset::EvaluatedPreset& print_preset)
{
    auto it = print_preset.values.find("layer_height");
    ASSERT(it != print_preset.values.end());
    append_value(values, "print.layer_height", it->second);
}

void append_tool_values(Expr::ValueMap& values, const Domain::Preset::HwToolConfig& tool)
{
    append_values(values, "tool.", tool.features);
}

}


