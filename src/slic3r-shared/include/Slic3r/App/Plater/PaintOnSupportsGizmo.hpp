///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ClipperPresenter.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Scene {
class Clipper;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::App::Plater {
class PaintOnSupportsDialog;
class PlaterScenePresenter;

// Please implement me!
class PaintOnSupportsGizmo : public Scene::IToolGizmo
{
public:
    PaintOnSupportsGizmo(
        Render::Device& device,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor* project_interactor
    );

    void on_activated() override;
    void on_deactivated() override;

    void on_project_activated(size_t new_project_id) override;
    void on_project_deactivated(size_t old_project_id) override;

    Scene::ToolType type() const override;
    Yoga::GizmoDialog* ui_dialog() override;

    void provide_clipper(Scene::Clipper& clipper) override;

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

private:
    std::unique_ptr<PaintOnSupportsDialog> m_dialog;

    Render::Device& m_device;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor* m_project_interactor{nullptr};

    Scene::ClipperPresenter m_clipper_presenter;
};

} // namespace Slic3r::App::Plater
