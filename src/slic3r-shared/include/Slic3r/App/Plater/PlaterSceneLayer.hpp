#pragma once

#include <cstdint>
#include <limits>


namespace Slic3r::App::Plater {

/**
 * @brief Plater-specific 3D scene layers.
 *
 * Use these to specify layer indices as used by IRenderLayerObject. Each layer can have
 * its layer start/end custom rendering code (e.g. to enable/disable blending, depth-test, etc.).
 */
enum class PlaterSceneLayer : int8_t {
    DocumentObjects = 0,
    GizmoHandles = 1,
    AlwaysOnTop = std::numeric_limits<int8_t>::max()
};

}
