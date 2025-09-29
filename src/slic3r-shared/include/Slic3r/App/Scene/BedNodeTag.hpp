#pragma once

#include <cstdint>
#include <cstddef>

namespace Slic3r::App::Scene {

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
    Axis,
    AxesMain,
    AxesScaler,
    Label,
};

/**
  * @brief Node tag for beds
  */
struct BedNodeTag
{
    const std::size_t config_container_id{ 0 };
    const std::size_t instance_id{ 0 };
    const BedElementType type{ BedElementType::Undefined };
};

} // namespace Slic3r::App::Scene
