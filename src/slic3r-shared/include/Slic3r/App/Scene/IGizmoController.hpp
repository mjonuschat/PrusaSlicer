#pragma once

#include "Slic3r/Domain/PrinterTechnology.hpp"

#include <cstdint>

namespace Slic3r::App::Scene {

enum class ToolType : uint8_t;

class IGizmoController
{
public:
    virtual ~IGizmoController() = default;

    virtual void deactivate_current_tool()                                  = 0;
    virtual void activate_tool(ToolType tool, Domain::PrinterTechnology pt) = 0;
};

} // namespace Slic3r::App::Scene
