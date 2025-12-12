#pragma once

#include "Slic3r/Uuid.hpp"
#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"


namespace Slic3r::Biz::Preset::IO {

constexpr const char* FEATURE_BASED_ID = "based_id";
constexpr const char* FEATURE_BASED_ROOT_ID = "based_root_id";

namespace Details {

Domain::Expr::ExprAst and_chain_exprs(const Domain::Preset::Expressions& exprs);
Domain::Preset::PresetValueMap config_box_to_values(const Domain::ConfigBox& cfg);
void save_transformed_preset_as_user(
    const Domain::Preset::RootPresetNode& root_preset,
    const std::string& path
);

template <typename FdmConfig, typename SlaConfig>
Domain::Preset::RootPresetNode transform_for_saving(
    const Domain::Preset::EvaluatedPreset<FdmConfig, SlaConfig>& source
)
{
    using namespace Domain::Preset;
    RootPresetNode ret;

    ret.kind = source.kind;
    ret.origin = PresetOrigin::User;
    ret.id = source.root_id;
    ret.user_file = source.user_file;
    auto it = source.features.find(FEATURE_BASED_ROOT_ID);
    if (it != source.features.end()) {
        ret.inherits = {std::get<std::string>(it->second)};
    }
    PresetNode main;

    main.condition = SourceLocatedExpr{Details::and_chain_exprs(source.conditions)};
    main.id = source.id;
    main.name = source.name;
    main.features = source.features;
    main.values = config_box_to_values(source.config_box());

    ret.variants.emplace_back(main);

    return ret;
}

} // namespace Details


template <typename FdmConfig, typename SlaConfig>
void save_evaluated_preset_as_user(
    const Domain::Preset::EvaluatedPreset<FdmConfig, SlaConfig>& preset,
    const std::string& path
)
{
    Details::save_transformed_preset_as_user(Details:: transform_for_saving(preset), path);
}

std::string preset_file_prefix(Domain::Preset::PresetKind kind);



} // namespace Slic3r::Biz::Preset::IO

