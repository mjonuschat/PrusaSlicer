#include "Slic3r/Biz/Preset/PresetEvaluator.hpp"

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
        m_named_presets.find(kind)->second.emplace(std::make_pair(node.id, node_path));
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

PresetEvaluator::EvaluatedPrinterPreset PresetEvaluator::evaluate(PresetKind kind, const HwPrinterConfig& hw_config) const
{
    auto it = m_presets.find(kind);
    ASSERT(it != m_presets.end());

    // 1. build variables
    // 2. loop thru profiles and match conditions

    return {};
}

}