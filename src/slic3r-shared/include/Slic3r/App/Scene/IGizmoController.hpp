#pragma once

namespace Slic3r::App::Scene {

class IGizmoController
{
public:
    virtual ~IGizmoController() = default;

    virtual void deactivate_current_tool() = 0;
};

} // namespace Slic3r::App::Scene
