///|/ Copyright (c) Prusa Research 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/MeasureGizmo.hpp"

#include "Slic3r/App/Plater/MeasureDialog.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

MeasureGizmo::MeasureGizmo()
{
    m_dialog = std::make_unique<MeasureDialog>();

    m_dialog->on_copy() = [this]() {
        // just for test
        m_dialog->set_measure(MeasureDialog::MeasureType::Distance, 67.491f);
    };
}

void MeasureGizmo::on_activated()
{
    m_dialog->spot1().set_as_edge(0.439f);
    m_dialog->spot2().set_as_plane();

    m_dialog->set_measure(MeasureDialog::MeasureType::Angle, 67.491f);
}

void MeasureGizmo::on_deactivated()
{

}

Scene::ToolType MeasureGizmo::type() const
{
    return Scene::ToolType::MeasureGizmo;
}

Yoga::Dialog* MeasureGizmo::unload_ui_dialog()
{
    return m_dialog.get();
}

Scene::GizmoActivationState MeasureGizmo::on_mouse(Scene::GizmoEventContext &ctx, bool only_active)
{
    return Scene::GizmoActivationState::Inactive;
}

}
