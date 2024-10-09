#pragma once

#include "Slic3r/App/Render/CommandBuffer.hpp"
#include "Slic3r/App/Scene/IRenderLayerObject.hpp"
#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Scene/Material.hpp"

namespace Slic3r::App::Scene {

class Node;

class IRenderNodeComponent : public IRenderLayerObject {
public:
    virtual void render(const Node& node, const Camera& camera, const Material& material_override, Render::CommandBuffer& cmd_buffer) const = 0;
};

}

