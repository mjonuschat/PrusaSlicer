///|/ Copyright (c) Prusa Research 2025
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Plater {
class MeasureDialog;

// Please implement me!
class MeasureGizmo : public Scene::IToolGizmo
{
public:
    MeasureGizmo();

    void on_activated() override;
    void on_deactivated() override;

    Scene::ToolType type() const override;
    Yoga::Dialog* unload_ui_dialog() override;

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

private:
    std::unique_ptr<MeasureDialog> m_dialog;
};

} // namespace Slic3r::App::Plater
