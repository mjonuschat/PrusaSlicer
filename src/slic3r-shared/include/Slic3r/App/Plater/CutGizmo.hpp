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
class GeometryDataFactory;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Plater {
class CutDialog;
class PlaterScenePresenter;

using namespace Slic3r::Domain;

// Please implement me!
class CutGizmo : public Scene::IToolGizmo
{
    using ModelGeometryManager     = Render::GeometryManager<CutAuxiliaryElementId>;
    using ModelTriangleMeshManager = Scene::TriangleMeshManager<CutAuxiliaryElementId>;

public:
    CutGizmo(
        Render::Device& device,
        Scene::GeometryDataFactory& data_factory,
        PlaterScenePresenter& scene_presenter,
        Biz::ProjectInteractor* project_interactor
    );

    /**
     * @name Implementation of IGizmo interface
     * @{
     */
    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;
    void on_transient_mouse(Scene::GizmoEventContext& ctx) override;
    void on_cycle_prepare() override;
    /**@}*/

    /**
     * @name Implementation of IToolGizmo interface
     * @{
     */
    void on_activated() override;
    void on_deactivated() override;
    Scene::ToolType type() const override;
    Yoga::GizmoDialog* ui_dialog() override;
    /**@}*/

private:
    void init_scene_nodes();

    // Get transformed bounginh box of selected for cut object
    BoundingBoxf3 transformed_bounding_box(
        const Vec3d& plane_center,
        const Transform3d& rotation_m = Transform3d::Identity()
    ) const;
    // Set value for plane_center and update transformed bounding box if needed
    void set_plane_center(const Vec3d& center_pos, bool update_tbb = false);

    Vec3d mouse_position_in_local_plane(AxisType axis, const Domain::Line3d& mouse_ray) const;
    void dragging_handle_rotation(const Domain::Line3d& mouse_ray);

    void update_scene_nodes();

    void build_cut_plane_node(Scene::NodeBuilder& builder);
    void build_handles_nodes(Scene::NodeBuilder& builder);

    void rotate_vec3d_around_plane_center(Vec3d& vec);

    void update_cut_plane_mesh();
    void update_cut_plane_trafo();

    void update_handles_nodes(Handle hovered_handle = Handle());
    void update_handles_material_and_enability(Handle hovered_handle);
    void update_handles_local_fransform(Handle hovered_handle);
    void update_clipping_nodes_enability(bool connectors_editing);
    void update_clipped_mesh_material();

    void build_cut_part_mesh(
        CutPartNodeTag::Type type,
        size_t part_id,
        std::shared_ptr<const Slic3r::Domain::TriangleMesh> mesh,
        const Transform3d& trafo,
        Scene::NodeBuilder& builder
    );
    void reset_cut_part_meshes();
    void reset_handles_nodes();
    void reset();

    void set_enabled_scene_nodes(bool enabled);

    Domain::Transform3d get_cut_matrix();
    void flip_cut_plane();

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

    void clear_highlight();
    void on_stop_dragging();

private:
    std::unique_ptr<CutDialog> m_dialog;

    ModelGeometryManager m_model_geometry_manager;
    ModelTriangleMeshManager m_model_triangle_mesh_manager;
    Scene::Node* m_main_node{nullptr};
    Scene::Node* m_handles_node{nullptr};

    Domain::Vec3d m_plane_center;
    Domain::Vec3d m_old_center;
    Domain::Vec3d m_cut_normal;

    // workaround for using of the clipping plane normal
    Domain::Vec3d m_clp_normal{ Vec3d::Ones() };


    // transformed boundign box of selected inxtance
    Domain::BoundingBox3d m_transformed_bbox;

    // Meaning size of the modified elements.
    // Is used for maximim size of groove/connectors ets
    double m_mean_size;

    // used by dragging
    Handle m_hovered_handle;
    bool m_is_plane_hovered{false};

    bool m_can_flip_plane{ false };// indicates if plane was just clicked without dragging

    Domain::Transform3d m_rotation_m{Domain::Transform3d::Identity()};
    double m_snap_step{1.0};
    int m_connectors_group_id;

    // archived values
    // Vec3d m_ar_plane_center{Vec3d::Zero()};
    Transform3d m_start_dragging_m{Transform3d::Identity()};

    // data to check position of the cut palne center on gizmo activation
    Vec3d m_min_pos{Vec3d::Zero()};
    Vec3d m_max_pos{Vec3d::Zero()};
    Vec3d m_bb_center{Vec3d::Zero()};
    Vec3d m_center_offset{Vec3d::Zero()};

    BoundingBoxf3 m_bounding_box;
    BoundingBoxf3 m_transformed_bounding_box;

    double m_handle_radius{0.0};
    double m_handle_connection_len{0.0};
    Vec3d m_cut_plane_start_move_pos{Vec3d::Zero()};

    double m_snap_coarse_in_radius{0.0};
    double m_snap_coarse_out_radius{0.0};
    double m_snap_fine_in_radius{0.0};
    double m_snap_fine_out_radius{0.0};

    // dragging angel in hovered axes
    double m_angle{0.0};

    CutPartSelection m_part_selection;

    double m_radius{0.0};
    float m_contour_width{0.4f};
    // float m_cut_plane_radius_koef{ 1.5f };
    // float m_shortcut_label_width{ -1.f };
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
    Scene::GeometryDataFactory& m_data_factory;
    PlaterScenePresenter& m_scene_presenter;
    Biz::ProjectInteractor* m_project_interactor{nullptr};
    bool m_activated{false};
    bool m_dragging{false};
    bool m_highlighted{false};

    Scene::Ray m_translation_ray;
    double m_start_t{0};

    Scene::Node::NodeList m_handles; // list of handles
};

} // namespace Slic3r::App::Plater
