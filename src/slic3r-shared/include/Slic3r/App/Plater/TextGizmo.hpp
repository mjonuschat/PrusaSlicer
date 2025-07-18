///|/ Copyright (c) Prusa Research 2025
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once
#include "Slic3r/App/Scene/IGizmo.hpp" // IToolGizmo
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Emboss/IFontManager.hpp"
#include "Slic3r/Biz/Emboss/StyleManager.hpp"

namespace Slic3r::App::Yoga {
class Dialog;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {
class TextDialog;

// Please implement me!
class TextGizmo : public Scene::IToolGizmo
{
public:
    TextGizmo(
        Render::Device& device,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor& project_interactor,
        Biz::Emboss::IFontManager& font_manager,
        Scene::GizmoManager& gizmo_manager
    );
    std::unique_ptr<Yoga::GizmoWindow> release_ui_window() override;

    /**
     * @name Implementation of IGizmo interface
     */
    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
    void register_commands(Platform::CommandRegistry& registry) override;

    /**
     * @name Implementation of IToolGizmo interface
     */
    void on_activated() override;
    void on_deactivated() override;
    Scene::ToolType type() const override { return Scene::ToolType::Text; }

    /// <summary>
    /// Create new text without given position
    /// </summary>
    /// <param name="volume_type">Object part / Negative volume / Modifier</param>
    bool add_text_by_view_direction(Domain::ModelVolumeType volume_type);

    void update_layout(bool show_for_part);
    // Only debug 
    void render_imgui();
private:
    void close();
    bool init_create(Domain::ModelVolumeType volume_type);

    void update_presets_list();
    void activate_preset(/*preset*/);

    Render::Device& m_device;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor& m_project_interactor;
    Biz::Emboss::IFontManager& m_font_manager;
    Scene::GizmoManager& m_gizmo_manager;

    Biz::Emboss::StyleManager m_style_manager;

    Domain::TextConfiguration m_text_configuration = {};
    Domain::EmbossProjection m_projection = {};
    
    Yoga::Passthrough<TextDialog> m_dialog;
    Biz::ProjectInteractor& m_project_interactor;
};

} // namespace Slic3r::App::Plater
