#pragma once

#include "Slic3r/Domain/ModelVolume.hpp"
#include "Slic3r/Domain/Color.hpp"

namespace Slic3r::App::Scene {

static const std::unordered_map<Domain::ModelVolumeType, Domain::ColorRGBA> VOLUME_COLORS = {
    {Domain::ModelVolumeType::MODEL_PART,         {1.0f, 0.5f, 0.0f, 1.0f}},
    {Domain::ModelVolumeType::NEGATIVE_VOLUME,    {0.5f, 0.5f, 0.5f, 0.5f}},
    {Domain::ModelVolumeType::SUPPORT_BLOCKER,    {1.0f, 0.2f, 0.2f, 0.5f}},
    {Domain::ModelVolumeType::SUPPORT_ENFORCER,   {0.2f, 0.2f, 1.0f, 0.5f}},
    {Domain::ModelVolumeType::PARAMETER_MODIFIER, {1.0f, 1.0f, 0.2f, 0.5f}},
    {Domain::ModelVolumeType::INVALID,            {1.0f, 0.2f, 0.2f, 0.5f}},
};

} // namespace Slic3r::App::Scene
