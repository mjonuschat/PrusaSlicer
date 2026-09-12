///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/ Copyright (c) OrcaSlicer 2025 SoftFever @SoftFever
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
#include "boss/features/aligned-rear-seam/AlignedRearSeamFeature.hpp"

#include <algorithm>
#include <cstddef>

#include "Slic3r/Domain/Config.hpp"
#include "boss/foundation/SeamVisibilityContext.hpp"

namespace Slic3r::Boss {

void AlignedRearSeamFeature::modify_visibility(const SeamVisibilityContext &ctx)
{
    if (ctx.config == nullptr || !ctx.config->get<bool>("seam_position_aligned_rear"))
        return;

    for (std::size_t i = 0; i < ctx.visibility.size(); ++i) {
        const float rear_bias =
            std::clamp((ctx.normals[i].dot(Vec3f(0.0f, -1.0f, 0.0f)) + 1.2f) * 0.5f, 0.0f, 1.0f);
        ctx.visibility[i] += rear_bias;
    }
}

} // namespace Slic3r::Boss
