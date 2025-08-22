///|/ Copyright (c) Prusa Research 2025
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once
#include <optional>
#include "Slic3r/App/Scene/IGizmo.hpp" // IToolGizmo
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Emboss/IFontManager.hpp"
#include "Slic3r/Biz/Emboss/TextPresetManager.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp" // ISceneSelectionChangedListener

namespace Slic3r::App::Yoga {
class Dialog;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Plater {
class TextDialog;

/**
 *  @brief   Tool for emboss text on surface of model
 *  @details Main idea: 
 *  1) Gizmo is open only with selected text volume (detail in 'on_activated').
 *  2) Shown data are from m_preset_manager cache (actualized in 'on_scene_selection_changed')
 *  3) ModelVolume always contain TextConfiguration to recreate volume (without modification)
 *  4) Volume is created in a process thread (detail inside file 'EmbossJob'). 
 */
class TextGizmo : 
    public Scene::IToolGizmo,
    public Biz::Scene::ISceneSelectionChangedListener
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

    /**
     * @name Implementation of ISceneSelectionChangedListener interface
     */
    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection) override;

    /// <summary>
    /// Create new text without given position
    /// </summary>
    /// <param name="volume_type">Object part / Negative volume / Modifier</param>
    bool add_text_by_view_direction(Domain::ModelVolumeType volume_type);

    // Only debug 
    void render_imgui();
private:
    // Params to change inside of volume after create, which are not in preset manager
    struct UpdateParams {
        std::optional<Domain::Transform3d> volume_transformation;
        std::optional<Domain::ModelVolumeType> volume_type;
    };
    // Call every time when param of emboss change
    bool update_volume(const UpdateParams& params = UpdateParams{});
    void close();

    bool init_create(Domain::ModelVolumeType volume_type);
    bool emboss_text(Domain::ModelVolumeType volume_type, const Scene::Ray& ray, const Scene::NodePickResults& results);

    Render::Device& m_device;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor& m_project_interactor;
    Biz::Emboss::IFontManager& m_font_manager;
    Scene::GizmoManager& m_gizmo_manager;

    Biz::Emboss::TextPresetManager m_preset_manager;

    std::string m_text; // embossed text
    Yoga::Passthrough<TextDialog> m_dialog;

    // only for check
    Domain::ObjectID last_loaded_volume_id;
};

} // namespace Slic3r::App::Plater
