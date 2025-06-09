///|/ Copyright (c) Prusa Research 2025
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once
#include "Slic3r/App/Scene/IGizmo.hpp" // IToolGizmo
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::Biz {
    class ProjectInteractor;
}

namespace Slic3r::App::Yoga {
class Dialog;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {
class TextDialog;

// Please implement me!
class TextGizmo : public Scene::IToolGizmo
{
public:
    using CloseFn = std::function<void()>;
    TextGizmo(
        Render::Device& device,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor& project_interactor,
        CloseFn close_fn
    );
    std::unique_ptr<Yoga::GizmoWindow> release_ui_window() override;

    /**
     * @name Implementation of IGizmo interface
     */
    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

    /**
     * @name Implementation of IToolGizmo interface
     */
    void on_activated() override;
    void on_deactivated() override;
    bool enabled() const override;
    Scene::ToolType type() const override;
    void update_layout(bool show_for_part);
    void render_imgui() const;

private:
    void update_presets_list();
    void activate_preset(/*preset*/);

    Render::Device& m_device;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor& m_project_interactor;
    CloseFn m_close_fn; // call GizmoManager to close current gizmo
    Yoga::Passthrough<TextDialog> m_dialog;
    Biz::ProjectInteractor& m_project_interactor;
};

} // namespace Slic3r::App::Plater
