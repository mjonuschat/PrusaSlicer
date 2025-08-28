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
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Emboss/IFontManager.hpp"
#include "Slic3r/Biz/Emboss/TextPresetManager.hpp"
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
    public Scene::IMouseDrag
{
public:
    TextGizmo(
        Render::Device& device,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor& project_interactor,
        Biz::Emboss::IFontManager& font_manager,
        Scene::GizmoManager& gizmo_manager
    );
    ~TextGizmo();
    std::unique_ptr<Yoga::GizmoWindow> release_ui_window() override;

    /**
     * @name Implementation of IGizmo interface
     */
    std::unique_ptr<Yoga::GizmoWindow> release_ui_window() override;
    void register_commands(Platform::CommandRegistry& registry) override;
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
    void on_activated() override;
    void on_deactivated() override;
    Scene::ToolType type() const override { return Scene::ToolType::Text; }

    /**
     * @name Implementation of ISceneSelectionChangedListener interface
     */
    void on_scene_selection_changed(Domain::SelectionId project_id, const Biz::Scene::ObjectSelection& selection) override;

    /**
     *  @brief  Create new text volume without given position
     *  @param  volume_type - volume type(part/negative/modifier)
     *  @retval             - True on success otherwise False.
     */
    bool add_text_by_view_direction(Domain::ModelVolumeType volume_type = Domain::ModelVolumeType::MODEL_PART);

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
    void rotate(double absolut_angle);
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

    struct Scale {
        std::optional<float> width;
        std::optional<float> height;
        std::optional<float> depth;
        double char_gap = 1.;
        double line_gap = 1.;
    };
    Scale m_volume_scale;
    bool calc_scale(const Domain::Project& project, const Domain::ElementRef& ref); // True when exist change in scale otherwise false

    bool m_use_inch = false;
    bool m_use_deg = true;

    struct Drag; // like pimpl
    std::unique_ptr<Drag> m_drag; // exist only during drag operation

    // only for check
    Domain::ObjectID m_last_loaded_volume_id;
};

// TODO: move function to surface drag utility
// Calculate volume rotation around embossed axis VRT Y as up vector(zero angle)
std::optional<float> calc_rotation(const Domain::Project& project, const Domain::ElementRef& ref);
std::optional<float> calc_distance();

} // namespace Slic3r::App::Plater
