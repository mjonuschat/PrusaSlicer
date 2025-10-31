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

    struct Callbacks
    {
        std::function<void(double ratio)> clipping_view_ratio_changed{ nullptr };
    };

    Callbacks& callbacks();

private:
    Yoga::ButtonGroup m_group_tool;
    Yoga::ButtonGroup m_group_shape;

    Callbacks m_callbacks;
};

} // namespace Slic3r::App::Plater
