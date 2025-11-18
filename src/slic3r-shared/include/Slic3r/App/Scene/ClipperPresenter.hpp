#pragma once

#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/ClipperPresenterHelper.hpp"
#include "Slic3r/App/Scene/IGizmo.hpp"

namespace Slic3r::Domain {
class ModelObject;
class ModelInstance;
} // namespace Slic3r::Domain

namespace Slic3r::App::Render {
class Device;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Scene {

class Clipper;
class Scene;
class Node;

class ClipperPresenter
{
    using ModelGeometryManager     = Render::GeometryManager<ClipperElement>;
    using ModelTriangleMeshManager = TriangleMeshManager<ClipperElement>;

public:
    ClipperPresenter() {};
    ClipperPresenter(Clipper* clipper, Render::Device* device);

    void activate(
        Scene* scene,
        const Domain::ModelObject* selected_object,
        const Domain::ModelInstance* selected_instance,
        double sla_shift = 0.
    );
    void deactivate(bool force_enabled_scene_nodes = true);
    void reset();
    void show_clipper(bool show);

    void set_position_by_ratio(double pos, bool keep_normal);
    void set_range_and_pos(const Domain::Vec3d& cpl_normal, double cpl_offset, double pos);
    void set_behavior(bool hide_clipped, bool fill_cut, double contour_width);
    int is_projection_inside_cut(const Domain::Vec3d& point_in) const;

    void set_clickable_plane(bool clickable);
    void set_enable_mesh(bool enable);
    void set_enable_plane(bool enable);
    void set_enable_contour(bool enable);
    void set_color_mesh(Slic3r::Domain::ColorRGBA color);
    void set_color_plane(Slic3r::Domain::ColorRGBA color);
    void set_color_contour(Slic3r::Domain::ColorRGBA color);

    void reset_ignored();
    void add_ignored(size_t volume_id, size_t island_id);

    bool unproject_on_cut_plane(const Ray& ray, Domain::Vec3d& pos, Domain::Vec3d& pos_world, bool respect_contours = true);

private:
    void init_main_node();
    // build Mesh nodes from selected instance
    void build_meshes_nodes(const Domain::Transform3d& inst_trafo);
    // build Plane/Contour nodes from Clipper
    void build_non_mesh_node(
        ClipperElementType type,
        const indexed_triangle_set& its,
        size_t clipper_id,
        size_t island_id
    );
    // update all nodes (rescreate Plane/Contour nodes and update clip for Mesh nodes) from Clipper
    void update_nodes();

private:
    Clipper* m_clipper{nullptr};
    Scene* m_scene{nullptr};
    Render::Device* m_device{nullptr};

    ModelGeometryManager m_model_geometry_manager;
    ModelTriangleMeshManager m_model_triangle_mesh_manager;

    struct ClipperId
    {
        size_t id;
        size_t island_id;

        bool operator==(const ClipperId& rhs) const
        {
            return id == rhs.id && island_id == rhs.island_id;
        }
    };

    std::vector<ClipperId> m_ignored_ids;

    Node* m_main_node{nullptr};

    Slic3r::Domain::ColorRGBA m_mesh_color{Slic3r::Domain::ColorRGBA::GRAY()};
    Slic3r::Domain::ColorRGBA m_plane_color{Slic3r::Domain::ColorRGBA::YELLOW()};
    Slic3r::Domain::ColorRGBA m_contour_color{Slic3r::Domain::ColorRGBA::WHITE()};

    bool m_mesh_enabled{true};
    bool m_plane_enabled{true};
    bool m_contour_enabled{true};

    Ray m_translation_ray;
    double m_start_t{ 0 };

};
} // namespace Slic3r::App::Scene
