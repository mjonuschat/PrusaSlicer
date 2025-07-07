///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Plater {
class PaintOnSupportsDialog;

// Please implement me!
class PaintOnSupportsGizmo : public Scene::IToolGizmo
{
public:
    PaintOnSupportsGizmo();

    void on_activated() override;
    void on_deactivated() override;

    Scene::ToolType type() const override;
    Yoga::Dialog* unload_ui_dialog() override;

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

private:
    std::unique_ptr<PaintOnSupportsDialog> m_dialog;
};

} // namespace Slic3r::App::Plater
