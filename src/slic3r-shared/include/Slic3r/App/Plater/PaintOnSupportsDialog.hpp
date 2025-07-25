///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/GizmoDialog.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

namespace Slic3r::App::Plater {

class PaintOnSupportsDialog : public Yoga::GizmoDialog
{
public:
    PaintOnSupportsDialog();

private:
    Yoga::ButtonGroup m_group_tool;
    Yoga::ButtonGroup m_group_shape;
};

} // namespace Slic3r::App::Plater
