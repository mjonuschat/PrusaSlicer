#pragma once

#include "Slic3r/App/Scene/Scene.hpp"

#include <cstdint>

namespace Slic3r::App::Scene {

enum class ToolType : uint8_t;

class IGizmoController
{
public:
    virtual ~IGizmoController() = default;

    virtual void deactivate_current_tool() = 0;

    virtual void activate_tool(ToolType tool) = 0;

    virtual ToolType current_tool_type() const = 0;

    virtual NodePickResults repick() const = 0;
};

} // namespace Slic3r::App::Scene
