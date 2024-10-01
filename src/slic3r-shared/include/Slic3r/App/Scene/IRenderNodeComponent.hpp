#pragma once

#include "Slic3r/App/Render/CommandBuffer.hpp"

namespace Slic3r::App::Scene {

class Node;

class IRenderNodeComponent {
public:
    virtual ~IRenderNodeComponent() = default;

    virtual void render(const Node& node, Render::CommandBuffer& cmd_buffer) = 0;
};

}

