// src/libslic3r/src/libslic3r/boss/surface/fuzzyskin/FuzzySkinNoiseProvider.hpp
#pragma once

#include "boss/features/structured-fuzzy-skin/StructuredFuzzySkinFeature.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::Boss {

struct FuzzySkinNoiseProvider {
    // Returns a displacement in [-thickness, thickness] for the given
    // structured noise type, sampled at (sample_point, slice_z) in
    // unscaled mm. `type` must not be FuzzySkinNoiseType::Classic.
    static double get_displacement(
        Domain::Boss::FuzzySkinNoiseType type,
        double                           feature_size,
        int                              octaves,
        double                           persistence,
        const Point                     &sample_point,
        double                           slice_z,
        double                           thickness);
};

} // namespace Slic3r::Boss
