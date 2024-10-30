#pragma once

#include "Slic3r/App/Plater/IGizmo.hpp"

namespace Slic3r::App::Plater {

class QuickDragGizmo : public IGizmo {
public:
    GizmoActivationState on_mouse(const GizmoEventContext& ctx, bool only_active) override;

private:
};

}
