///|/ Copyright (c) Prusa Research 2025
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {
class Dialog;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {
class TextDialog;

// Please implement me!
class TextGizmo : public Scene::IToolGizmo
{
public:
    TextGizmo();

    void on_activated() override;
    void on_deactivated() override;

    Scene::ToolType type() const override;
    std::unique_ptr<Yoga::GizmoWindow> release_ui_window() override;

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

    void update_layout(bool show_for_part);

private:
    void update_presets_list();
    void activate_preset(/*preset*/);

private:
    Yoga::Passthrough<TextDialog> m_dialog;
};

} // namespace Slic3r::App::Plater
