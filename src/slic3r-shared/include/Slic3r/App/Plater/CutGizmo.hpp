///|/ Copyright (c) Prusa Research 2025  Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp" // ISceneSelectionChangedListener

namespace Slic3r::App::Yoga {
class GizmoDialog;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::Biz {
class ProjectInteractor;
} // namespace Slic3r::Biz

namespace Slic3r::Domain {
    class ModelObject;
    class ModelVolume;
    class ModelInstance;
}

namespace Slic3r::App::Plater {
class CutDialog;
class PlaterScenePresenter;

using namespace Slic3r::Domain;

class PartSelection {
public:
    PartSelection() = default;
    PartSelection(const ModelObject* mo, const Transform3d& cut_matrix, int instance_idx, const Vec3d& center, const Vec3d& normal/*, const CommonGizmosDataObjects::ObjectClipper& oc*/);
    PartSelection(const ModelObject* mo, int instance_idx_in);
    ~PartSelection() { m_model.clear_objects(); }

    struct Part {
        //GLModel glmodel;
        //MeshRaycaster raycaster;
        bool selected;
        bool is_modifier;
    };

    //    void render(const Vec3d* normal, GLModel& sphere_model);
    void toggle_selection(const Vec2d& mouse_pos);
    void turn_over_selection();
    ModelObject* model_object() { return m_model.objects.front(); }
    bool valid() const { return m_valid; }
    bool is_one_object() const;
    const std::vector<Part>& parts() const { return m_parts; }
    const std::vector<size_t>* get_ignored_contours_ptr() const { return (valid() ? &m_ignored_contours : nullptr); }

    std::vector<Part> get_cut_parts();

private:
    Model m_model;
    int m_instance_idx;
    std::vector<Part> m_parts;
    bool m_valid = false;
    std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>> m_contour_to_parts; // for each contour, there is a vector of parts above and a vector of parts below
    std::vector<size_t> m_ignored_contours; // contour that should not be rendered (the parts on both sides will both be parts of the same object)

    std::vector<Vec3d> m_contour_points;         // Debugging
    std::vector<std::vector<Vec3d>> m_debug_pts; // Debugging

    void add_object(const ModelObject* object);
};
// Please implement me!
class CutGizmo : public Scene::IToolGizmo, public Biz::Scene::ISceneSelectionChangedListener
{
public:
    CutGizmo(
        Render::Device& device,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor& project_interactor
    );

    void on_activated() override;
    void on_deactivated() override;

    Scene::ToolType type() const override;
    Yoga::GizmoDialog* ui_dialog() override;

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

    /**
     * @name Implementation of ISceneSelectionChangedListener interface
     */
    void on_scene_selection_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection& selection
    ) override;

private:
    Domain::Transform3d get_cut_matrix();

    bool can_perform_cut() const;
    void perform_cut();

    void apply_connectors_in_model(Domain::ModelObject* mo, int& dowels_count);

    void apply_cut_connectors(Domain::ModelObject* mo, const std::string& connector_name);


private:
    std::unique_ptr<CutDialog> m_dialog;

    Domain::Vec3d m_plane_center;
    Domain::Vec3d m_old_center;
    Domain::Vec3d m_cut_normal;

    Domain::Transform3d m_rotation_m{ Domain::Transform3d::Identity() };

    PartSelection m_part_selection;

    Domain::ModelObject* m_selected_object{ nullptr };
    Domain::ModelInstance* m_selected_instance{nullptr};

    // should be in Gizmo
    bool m_keep_upper{ true };
    bool m_keep_lower{ true };
    bool m_keep_as_parts{ false };
    bool m_place_on_cut_upper{ true };
    bool m_place_on_cut_lower{ false };
    bool m_flip_upper{ false };
    bool m_flip_lower{ false };

    Render::Device& m_device;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor& m_project_interactor;
};

} // namespace Slic3r::App::Plater
