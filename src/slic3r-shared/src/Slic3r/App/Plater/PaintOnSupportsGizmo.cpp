///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/PaintOnSupportsGizmo.hpp"

#include "Slic3r/App/Plater/PaintOnSupportsDialog.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

PaintOnSupportsGizmo::PaintOnSupportsGizmo()
{
    m_dialog = std::make_unique<PaintOnSupportsDialog>();
}

void PaintOnSupportsGizmo::on_activated()
{

}

void PaintOnSupportsGizmo::on_deactivated()
{

}

Scene::ToolType PaintOnSupportsGizmo::type() const
{
    return Scene::ToolType::PaintOnSupportsGizmo;
}

Yoga::Dialog* PaintOnSupportsGizmo::unload_ui_dialog()
{
    return m_dialog.get();
}

Scene::GizmoActivationState PaintOnSupportsGizmo::on_mouse(Scene::GizmoEventContext &ctx, bool only_active)
{
    return Scene::GizmoActivationState::Inactive;
}

}
