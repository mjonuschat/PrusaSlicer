#pragma once

#include <cstdint>

namespace Slic3r::App::Plater {

/**
  * @brief Type of elements composing a bed
  */
enum class BedElementType : int8_t
{
    Undefined = 0,
    PlateDefault,
    PlateTextured,
    Contour,
    Grid,
    PrintVolume,
    Model,
    Axes
};

/**
  * @brief Node tag for beds
  */
struct BedNodeTag
{
    const size_t bed_id{ 0 };
    const size_t instance_id{ 0 };
    const BedElementType type{ BedElementType::Undefined };
};

} // namespace Slic3r::App::Plater