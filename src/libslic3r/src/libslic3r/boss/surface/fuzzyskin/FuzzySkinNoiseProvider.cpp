// src/libslic3r/src/libslic3r/boss/surface/fuzzyskin/FuzzySkinNoiseProvider.cpp
#include "FuzzySkinNoiseProvider.hpp"

#include <cmath>

#include <FastNoiseLite.h>

#include "Slic3r/Assert.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::Boss {

using Domain::Boss::FuzzySkinNoiseType;

double FuzzySkinNoiseProvider::get_displacement(
    const FuzzySkinNoiseType type,
    const double             feature_size,
    const int                octaves,
    const double             persistence,
    const Point             &sample_point,
    const double             slice_z,
    const double             thickness)
{
    ASSERT(type != FuzzySkinNoiseType::Classic,
           "FuzzySkinNoiseProvider never handles Classic -- that branch stays in native FuzzySkin.cpp's "
           "unchanged uniform-random jitter so the disabled/default path never reaches this provider");

    // thread_local, not a static/global: this state is scoped per slicing worker thread, so nothing is shared.
    thread_local FastNoiseLite fnl;
    thread_local FuzzySkinNoiseType last_type        = FuzzySkinNoiseType::Classic;
    thread_local double             last_feature_size = 0;
    thread_local int                last_octaves      = 0;
    thread_local double             last_persistence  = 0;

    if (type != last_type || feature_size != last_feature_size || octaves != last_octaves || persistence != last_persistence) {
        fnl.SetFrequency(1.0f / float(feature_size));

        switch (type) {
        case FuzzySkinNoiseType::Perlin:
            fnl.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
            fnl.SetFractalType(FastNoiseLite::FractalType_FBm);
            fnl.SetFractalOctaves(octaves);
            fnl.SetFractalLacunarity(2.0f);
            fnl.SetFractalGain(float(persistence));
            break;
        case FuzzySkinNoiseType::Billow:
            fnl.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            fnl.SetFractalType(FastNoiseLite::FractalType_FBm);
            fnl.SetFractalOctaves(octaves);
            fnl.SetFractalLacunarity(2.0f);
            fnl.SetFractalGain(float(persistence));
            break;
        case FuzzySkinNoiseType::RidgedMulti:
            fnl.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
            fnl.SetFractalType(FastNoiseLite::FractalType_Ridged);
            fnl.SetFractalOctaves(octaves);
            fnl.SetFractalLacunarity(2.0f);
            fnl.SetFractalGain(0.5f);
            break;
        case FuzzySkinNoiseType::Voronoi:
            fnl.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
            fnl.SetFractalType(FastNoiseLite::FractalType_None);
            fnl.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_EuclideanSq);
            fnl.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Sub);
            break;
        default:
            break;
        }

        last_type         = type;
        last_feature_size = feature_size;
        last_octaves      = octaves;
        last_persistence  = persistence;
    }

    const double x_mm = unscaled<double>(sample_point.x());
    const double y_mm = unscaled<double>(sample_point.y());

    double noise = fnl.GetNoise(float(x_mm), float(y_mm), float(slice_z));

    if (type == FuzzySkinNoiseType::Billow)
        noise = std::abs(noise) * 2.0 - 1.0;

    return noise * thickness;
}

} // namespace Slic3r::Boss
