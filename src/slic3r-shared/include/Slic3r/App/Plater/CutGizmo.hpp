///|/ Copyright (c) Prusa Research 2025  Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp" // ISceneSelectionChangedListener
#include "Slic3r/App/Plater/CutPartSelection.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/AuxiliaryElementId.hpp"
#include "Slic3r/App/Plater/CutNodeTag.hpp"

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
} // namespace Slic3r::Domain

namespace Slic3r::App::Scene {
class NodeBuilder;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Plater {
class CutDialog;
class PlaterScenePresenter;

using namespace Slic3r::Domain;

// Please implement me!
class CutGizmo : public Scene::IToolGizmo, public Biz::Scene::ISceneSelectionChangedListener
{
    using ModelGeometryManager     = Render::GeometryManager<Scene::AuxiliaryElementId>;
    using ModelTriangleMeshManager = Scene::TriangleMeshManager<Scene::AuxiliaryElementId>;

public:
    CutGizmo(
        Render::Device& device,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor* project_interactor
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
    void init_scene_nodes();
    void update_scene_nodes();

    void build_cut_plane_node(Scene::NodeBuilder& builder);
    void update_cut_plane_mesh();
    void update_cut_plane_trafo();
    void build_clipping_plane_node(Scene::NodeBuilder& builder);

    void build_cut_part_mesh(
        CutMeshType type,
        size_t part_id,
        std::shared_ptr<const Slic3r::Domain::TriangleMesh> mesh,
        const Transform3d& trafo,
        Scene::NodeBuilder& builder
    );
    void reset_cut_part_meshes();
    void reset();

    void set_enabled_scene_nodes(bool enabled);

    Domain::Transform3d get_cut_matrix();

    bool can_perform_cut() const;
    bool has_valid_groove() const;

    void perform_cut();

    void reset_cut_by_contours();

    void process_contours();

    void apply_connectors_in_model(Domain::ModelObject* mo, int& dowels_count);

    void apply_cut_connectors(Domain::ModelObject* mo, const std::string& connector_name);

    bool is_planar_mode() const;
    bool keep_as_parts() const;
    bool keep_upper() const;
    bool keep_lower() const;
    bool place_on_cut_upper() const;
    bool place_on_cut_lower() const;
    bool flip_upper() const;
    bool flip_lower() const;

private:
    std::unique_ptr<CutDialog> m_dialog;

    ModelGeometryManager m_model_geometry_manager;
    ModelTriangleMeshManager m_model_triangle_mesh_manager;
    Scene::Node* m_main_node{nullptr};

    Domain::Vec3d m_plane_center;
    Domain::Vec3d m_old_center;
    Domain::Vec3d m_cut_normal;

    // transformed boundign box of selected inxtance
    Domain::BoundingBox3d m_transformed_bbox;

    // Meaning size of the modified elements.
    // Is used for maximim size of groove/connectors ets
    double m_mean_size;

    Domain::Transform3d m_rotation_m{Domain::Transform3d::Identity()};

    CutPartSelection m_part_selection;

    double m_radius{0.0};
    Biz::Cut::Groove m_groove;
    // Vertices of the groove used to detection if groove is valid
    std::vector<Domain::Vec3d> m_groove_vertices;

    // Input params for cut with snaps
    float m_snap_bulge_proportion{0.15f};
    float m_snap_space_proportion{0.3f};

    Domain::ModelObject* m_selected_object{nullptr};
    const Domain::ModelInstance* m_selected_instance{nullptr};
    size_t m_instance_idx{size_t(-1)};

    bool m_groove_editing{false};
    bool m_imperial_units{false};
    bool m_cut_by_contour{false};

    Render::Device& m_device;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor* m_project_interactor{nullptr};
};

} // namespace Slic3r::App::Plater
