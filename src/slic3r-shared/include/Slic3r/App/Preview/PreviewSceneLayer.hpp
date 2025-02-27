#pragma once

#include <cstdint>

namespace Slic3r::App::Preview {

/**
 * @brief Preview-specific 3D scene layers.
 *
 * Use these to specify layer indices as used by IRenderLayerObject. Each layer can have
 * its layer start/end custom rendering code (e.g. to enable/disable blending, depth-test, etc.).
 */
enum class PreviewSceneLayer : int8_t
{
    Toolpaths = 0,
    Options,
    ToolMarker,
    CogMarker,
};

} // Slic3r::App::Preview
