#include "Slic3r/Biz/Preset/PresetEvaluator.hpp"
#include "Slic3r/Biz/Preset/PresetCollectionEvaluator.hpp"
#include "Slic3r/Biz/Preset/ValueMapBuilder.hpp"

namespace Slic3r::Biz::Preset {


void PresetEvaluator::build_named_presets()
{
    m_named_presets.clear();
    for (const auto& [kind, presets] : m_presets)
        for (const auto& p : presets)
            collect_named_presets(kind, p, {&p});
}

void PresetEvaluator::collect_named_presets(PresetKind kind, const PresetNode& node, const PresetNodePath& node_path)
{
    if (!node.id.empty()) {
        m_named_presets[kind].emplace(std::make_pair(node.id, node_path));
    }

    for (const auto& v : node.variants) {
        PresetNodePath child_path = node_path;
        child_path.push_back(&v);
        collect_named_presets(kind, v, child_path);
    }
}

const Domain::Preset::PresetNode* PresetEvaluator::find_node(PresetKind kind, std::string_view name) const
{
    auto presets_it = m_presets.find(kind);
    if (presets_it == m_presets.end())
        return nullptr;
    const auto& presets = presets_it->second;
    auto it = std::find_if(presets.begin(), presets.end(), [&name](const auto& preset) { return preset.name == name; });
    if (it == presets.end())
        return nullptr;
    return &*it;
}


Domain::Preset::EvaluatedPreset PresetEvaluator::preset_from_context(PresetKind kind, const EvalPresetContext& context)
{
    return {
        .kind = kind,
        .id = context.id,
        .name = context.name,
        .values = context.values,
        .features = context.features,
        .conditions = context.conditions,
        .last_node_location = context.last_node_location
    };
}

PresetEvaluator::EvaluatedPrinterPreset PresetEvaluator::evaluate(const HwPrinterConfig& hw_config) const
{
    EvaluatedPrinterPreset ret;
    Expr::ValueMap printer_values;
    append_printer_values(printer_values, hw_config);

    ValueMaps printer_tools_values;
    for (const auto& tool : hw_config.tools) {
        Expr::ValueMap tool_values = printer_values;

        append_tool_values(tool_values, tool);
        printer_tools_values.emplace_back(tool_values);
    }

    // 1. Printer preset
    PresetKind printer_kind = Domain::Preset::printer_kind(hw_config.technology);
    auto printers_it = m_presets.find(printer_kind);
    auto printer_names_it = m_named_presets.find(printer_kind);
    ASSERT(printers_it != m_presets.end() && printer_names_it != m_named_presets.end());

    PresetCollectionEvaluator printer_eval(printers_it->second, printer_names_it->second, m_eval, {});
    auto printer_presets = printer_eval.eval_preset({printer_tools_values});
    ASSERT(printer_presets.size() == 1);
    ret.preset = preset_from_context(printer_kind, printer_presets[0]);

    // 2. Material
    PresetKind mat_kind = Domain::Preset::material_kind(hw_config.technology);
    auto mats_it = m_presets.find(mat_kind);
    auto mat_names_it = m_named_presets.find(mat_kind);
    ASSERT(mats_it != m_presets.end() && mat_names_it != m_named_presets.end());

    PresetCollectionEvaluator material_eval(mats_it->second, mat_names_it->second, m_eval, {});
    auto mat_presets = material_eval.eval_preset(printer_tools_values);
    for (const auto& mat : mat_presets) {
        ret.materials.emplace_back(preset_from_context(mat_kind, mat));
    }

    // 3. Print preset
    PresetKind print_kind = Domain::Preset::print_kind(hw_config.technology);
    auto prints_it = m_presets.find(print_kind);
    auto print_names_it = m_named_presets.find(print_kind);
    ASSERT(prints_it != m_presets.end() && print_names_it != m_named_presets.end());

    PresetCollectionEvaluator print_eval(prints_it->second, print_names_it->second, m_eval, {});
    auto print_presets = print_eval.eval_preset(printer_tools_values);

    // 4. Tool print presets
    // for each tool
    PresetKind tool_kind = Domain::Preset::tool_print_kind(hw_config.technology);
    auto tool_it = m_presets.find(tool_kind);
    auto tool_names_it = m_named_presets.find(tool_kind);
    ASSERT(tool_it != m_presets.end() && tool_names_it != m_named_presets.end());

    PresetCollectionEvaluator tool_eval(tool_it->second, tool_names_it->second, m_eval, {});
    for (const auto& print_preset : print_presets) {
        Domain::Preset::EvaluatedPreset evaluated_print_preset = preset_from_context(print_kind, print_preset);
        Domain::Preset::EvaluatedToolPrintPresets tools;

        Expr::ValueMap print_values = printer_values;
        append_print_values(print_values, evaluated_print_preset);

        for (const auto & tool : hw_config.tools) {
            Expr::ValueMap tool_values = print_values;
            append_tool_values(tool_values, tool);

            // evaluate all variants
            Domain::Preset::EvaluatedToolPrintPresetVariants eval_variants;
            auto tool_preset_variants = tool_eval.eval_preset({tool_values});
            for (const auto& tool_variant : tool_preset_variants)
                eval_variants.emplace_back(preset_from_context(tool_kind, tool_variant));
            tools.emplace_back(std::move(eval_variants));
        }

        ret.prints.emplace_back(std::move(evaluated_print_preset), std::move(tools));
    }

    return ret;
}

}