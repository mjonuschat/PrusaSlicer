#pragma once
#include <cstdint>

namespace Slic3r::App::Plater {

/**
 * @brief Type of render elements for CutGizmo
 */
enum class CutMeshType : int8_t
{
    Undefined = 0,
    UpperPart,
    LowerPart,
    Connector,
    Plane,
    Clip
};

/**
 * @brief Node tag for CutGizmo elements
 */
struct CutNodeTag
{
    const CutMeshType type{CutMeshType::Undefined};
    size_t cut_part_id{size_t(-1)};
    size_t connector_id{size_t(-1)};
};

} // namespace Slic3r::App::Plater
