#pragma once

#include "Slic3r/App/Render/CommandBuffer.hpp"
#include "Slic3r/App/Scene/IRenderLayerObject.hpp"
#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Scene/Material.hpp"

namespace Slic3r::App::Scene {

class Node;

/**
 * @brief Generic 3D object rendering interface
 */
class IRenderNodeComponent : public IRenderLayerObject {
public:
    /**
     * Render 3D object associated with node
     * @param node Node the render component belongs to (and uses its world transform)
     * @param camera Scene camera defining view and projection matrices.
     * @param material_override Potential material override coming for the node or its parents.
     * @param cmd_buffer Command buffer the render commands are passed to.
     */
    virtual void render(const Node& node, const Camera& camera, const Material& material_override, Render::CommandBuffer& cmd_buffer) const = 0;
};

}

