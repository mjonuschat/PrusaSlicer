///|/ Copyright (c) Prusa Research 2025
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once
#include <optional>
#include "Slic3r/App/Scene/IGizmo.hpp" // IToolGizmo, forward-declaration of Slic3r::App::Yoga::Dialog
#include "Slic3r/App/Scene/GizmoManager.hpp"
#include "Slic3r/App/Scene/MouseDragDetector.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Yoga/Item.hpp" // Passthrough
#include "Slic3r/Biz/ProjectScoped.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Emboss/IFontManager.hpp"
#include "Slic3r/Biz/Emboss/TextPresetManager.hpp"
#include "Slic3r/Biz/Emboss/TextLines.hpp"
#include "Slic3r/Biz/Emboss/SurfaceDrag.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp" // ISceneSelectionChangedListener

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
    public Biz::Scene::ISceneSelectionChangedListener,
    public Scene::IMouseDrag // surface dragging
{
public:
    TextGizmo(
        Render::Device& device,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor& project_interactor,
        Biz::Emboss::IFontManager& font_manager,
        Scene::GizmoManager& gizmo_manager
    );
    // NOTE: Destructor is defined because Lin&Mac need it for Drag pimpl idiom
    ~TextGizmo() override; 

    /**
     * @name Implementation of IGizmo interface
     */
    std::unique_ptr<Yoga::GizmoWindow> release_ui_window() override;
    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

    /**
     * @name Implementation of IMouseDrag interface
     */
    bool on_drag_start(const Scene::GizmoEventContext& ctx) override;
    bool on_dragging(const Scene::GizmoEventContext& ctx) override;
    void on_drag_finish() override;
    void on_drag_cancel() override;

    /**
     * @name Implementation of IToolGizmo interface
     */
    bool enabled() const override;
    void on_activated() override;
    void on_deactivated() override;
    void on_project_activated(size_t new_project_id) override;
    void on_project_deactivated(size_t old_project_id) override;

    Scene::ToolType type() const override;
    bool allows_activation_by_double_click(const Scene::GizmoEventContext& ctx) override;
    void render_imgui() override; // Draw crosshair

    /**
     * @name Implementation of ISceneSelectionChangedListener interface
     */
    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection) override;

private:
    /**
     *  @brief  Create new text volume
     *  When selected object add volume on object
     *  No-selection or Multiple-objects creates new object(with one text volume)
     *  @param  volume_type - volume type(part/negative/modifier)
     *  @retval             - True on success otherwise False.
     */
    bool add_text_to_scene(Domain::ModelVolumeType volume_type = Domain::ModelVolumeType::MODEL_PART);
    // Call every time when param of emboss change
    bool update_volume(std::optional<Domain::ModelVolumeType> volume_type = {});
    void close();
    void rotate(double absolut_angle_in_rad); // callback on_rotation_change
    bool init_create(Domain::ModelVolumeType volume_type);
    bool emboss_text(Domain::ModelVolumeType volume_type, const Scene::Ray& ray, const Scene::NodePickResults& results);

    Render::Device& m_device;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor& m_project_interactor;
    Biz::Emboss::IFontManager& m_font_manager;
    Scene::GizmoManager& m_gizmo_manager;

    Biz::Emboss::TextPresetManager m_preset_manager;
    Biz::Emboss::SurfaceDrag m_surface_drag;
    Biz::Emboss::TextLinesModel m_text_lines; // per glyph feature

    struct ProjectContext; // forward declaration
    // m_projects use pimpl to hide ProjectContext into cpp file
    std::unique_ptr<Biz::ProjectScoped<ProjectContext>> m_proj_ctxs;

    TextDialog& dialog() { return *m_dialog.get(); }
    Yoga::Passthrough<TextDialog> m_dialog;
};

// TODO: move function to surface drag utility
// Calculate volume rotation around embossed axis VRT Y as up vector(zero angle)
std::optional<float> calc_rotation(const Domain::Project& project, const Domain::ElementRef& ref);
std::optional<float> calc_distance(const Domain::Project& project, const Domain::ElementRef& ref, Scene::Node& root);

} // namespace Slic3r::App::Plater
