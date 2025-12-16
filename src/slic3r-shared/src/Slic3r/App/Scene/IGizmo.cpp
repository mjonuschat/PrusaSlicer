///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Scene/IGizmo.hpp"

#include "Slic3r/App/Yoga/GizmoWindow.hpp"

namespace Slic3r::App::Scene {

bool IToolGizmo::supports_printer(Domain::PrinterTechnology pt) const
{
    return true;
}

bool IToolGizmo::enabled() const
{
    return true;
}

std::unique_ptr<Yoga::GizmoWindow> IToolGizmo::release_ui_window()
{
    return nullptr;
}

IToolGizmo::Callbacks& IToolGizmo::callbacks() { return m_callbacks; }

} // namespace Slic3r::App::Scene
