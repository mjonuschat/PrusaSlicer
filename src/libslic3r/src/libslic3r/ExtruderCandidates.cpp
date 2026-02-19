#include "libslic3r/ExtruderCandidates.hpp"

namespace Slic3r::Biz::Slicing {

std::vector<unsigned int>
get_painting_extruders(const Domain::ModelObject& model_object, const unsigned int num_extruders)
{
    if (!model_object.is_mm_painted()) {
        return {};
    }

    using Domain::TriangleSelector::TriangleStateType;
    std::array<bool, static_cast<size_t>(TriangleStateType::Count)> used_facet_states{};
    for (const Domain::ModelVolume* volume : model_object.volumes) {
        if (volume->is_mm_painted()) {
            const std::vector<bool>& volume_used_facet_states{
                volume->mm_segmentation_facets.get_data().used_states
            };

            assert(volume_used_facet_states.size() == used_facet_states.size());
            for (size_t state_idx = 1; state_idx
                 < std::min(volume_used_facet_states.size(), used_facet_states.size());
                 ++state_idx)
            {
                used_facet_states[state_idx] |= volume_used_facet_states[state_idx];
            }
        }
    }

    std::vector<unsigned int> result;
    for (size_t state_idx = static_cast<size_t>(TriangleStateType::Extruder1);
         state_idx < used_facet_states.size();
         ++state_idx)
    {
        if (used_facet_states[state_idx] && state_idx <= num_extruders) {
            result.emplace_back(state_idx);
        }
    }

    return result;
}

static std::set<unsigned> get_extra_support_extruders(
    const Domain::ObjectSettings& object_settings,
    const Domain::PrintSettings& print_settings
) {
    const auto support_material{print_settings.items.opt("support_material").get<bool>()};
    const bool raft{print_settings.items.opt("raft_layers").get<int>() > 0};

    std::optional<Domain::ConfigItem> object_support_material_opt{object_settings.overrides.get("support_material")};
    const bool object_support_material{object_support_material_opt && (*object_support_material_opt).get<bool>()};
    if (!support_material && !object_support_material && !raft) {
        return {};
    }

    std::set<unsigned> result;

    for (const std::string& key :
         {"support_material_extruder", "support_material_interface_extruder"})
    {
        const std::optional<Domain::ConfigItem> object_supports_extruder_item{
            object_settings.overrides.get(key)
        };
        if (object_supports_extruder_item) {
            const auto object_supports_extruder{object_supports_extruder_item->get<int>()};
            if (object_supports_extruder > 0) {
                result.insert(static_cast<unsigned>(object_supports_extruder - 1));
                continue;
            }
        }
        const int print_supports_extruder{print_settings.items.opt(key).get<int>()};
        if (print_supports_extruder > 0) {
            result.insert(static_cast<unsigned>(print_supports_extruder - 1));
        }
    }
    return result;
}

static std::set<unsigned> get_extruder_candidates(
    const Domain::VolumeSettings& volume_settings,
    const Domain::ObjectSettings& object_settings,
    const Domain::PrintSettings& print_settings
)
{
    std::set<unsigned> result;

    const int object_default_extruder{object_settings.items.opt("extruder").get<int>()};
    const std::optional<Domain::ConfigItem> volume_default_extruder_item{
        volume_settings.overrides.get("extruder")
    };
    const std::optional<int> volume_default_extruder{
        volume_default_extruder_item ? std::optional{volume_default_extruder_item->get<int>()} :
                                       std::nullopt
    };
    for (const std::string& key :
         {"perimeter_extruder", "infill_extruder", "solid_infill_extruder"})
    {
        const std::optional<Domain::ConfigItem> volume_extruder{
            volume_settings.overrides.get(key)
        };
        if (volume_extruder && volume_extruder->get<int>() > 0) {
            result.insert(static_cast<unsigned>(volume_extruder->get<int>() - 1));
            continue;
        }
        const std::optional<Domain::ConfigItem> object_extruder{
            object_settings.overrides.get(key)
        };
        if (object_extruder && object_extruder->get<int>() > 0) {
            result.insert(static_cast<unsigned>(object_extruder->get<int>() - 1));
            continue;
        }
        if (volume_default_extruder && volume_default_extruder != 0) {
            result.insert(static_cast<unsigned>(*volume_default_extruder) - 1);
            continue;
        }
        if (object_default_extruder != 0) {
            result.insert(static_cast<unsigned>(object_default_extruder) - 1);
            continue;
        }
        result.insert(static_cast<unsigned>(print_settings.items.opt(key).get<int>()) - 1);
    }
    return result;
}

std::vector<unsigned>
get_extruder_candidates(const Domain::Model& model, const Domain::ConfigPackFDM& config)
{
    ASSERT(config.tool.size() > 0);
    const Domain::PrintSettings& print_settings{config.print};
    std::set<unsigned> extruders;
    for (const Domain::ModelObject* object : model.objects) {
        const Domain::ObjectSettings& object_settings{object->object_settings};

        for (const auto& pair : object->layer_config_ranges) {
            const Domain::VolumeSettings& volume_settings{pair.second};
            extruders.merge(
                get_extruder_candidates(volume_settings, object_settings, print_settings)
            );
        }

        for (const Domain::ModelVolume* volume : object->volumes) {
            using Domain::ModelVolumeType::MODEL_PART;
            using Domain::ModelVolumeType::PARAMETER_MODIFIER;
            if (volume->type() != MODEL_PART && volume->type() != PARAMETER_MODIFIER) {
                continue;
            }
            extruders.merge(
                get_extruder_candidates(volume->volume_settings, object_settings, print_settings)
            );
        }
        const std::vector<unsigned> painting_extruders{
            get_painting_extruders(*object, config.tool.size())
        };
        for (unsigned extruder : painting_extruders) {
            extruders.insert(extruder - 1);
        }

        std::set<unsigned> support_extruders{
            get_extra_support_extruders(object_settings, config.print)
        };
        extruders.merge(support_extruders);
    }

    std::vector<unsigned> result;
    for (unsigned extruder : extruders) {
        if (extruder < config.tool.size()) {
            result.push_back(extruder);
        }
    }
    return result;
}
} // namespace Slic3r::Biz::Slicing
