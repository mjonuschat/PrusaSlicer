///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Scene/IGizmo.hpp"

#include "Slic3r/App/Yoga/Dialog.hpp"

namespace Slic3r::App::Scene {

bool IToolGizmo::supports_printer(PrinterTechnology pt) const { return true; }

bool IToolGizmo::enabled() const { return true; }

std::unique_ptr<Yoga::Dialog> IToolGizmo::unload_ui_dialog() { return nullptr; }

} // namespace Slic3r::App::Scene
