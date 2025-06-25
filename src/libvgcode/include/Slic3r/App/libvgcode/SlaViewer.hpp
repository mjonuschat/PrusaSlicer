///|/ Copyright (c) Prusa Research 2016 - 2023 Oleksandra Iushchenko @YuSanka, Vojtech Bubník @bubnikv, Filip Sykala @Jony01, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matena @lukasmatena, Vojtech Král @vojtechkral
///|/ Copyright (c) 2019 Sijmen Schoon
///|/
///|/ libvgcode library is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "AbstractViewer.hpp"
#include "SegmentTemplate.hpp"
#include "Slic3r/App/libvgcode/SlaObjectNodeTag.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/AuxiliaryElementId.hpp"
#include "Slic3r/Domain/ObjectID.hpp"

namespace Slic3r::Domain {
class TriangleMesh;
}

namespace Slic3r::Biz::Slicing::Sla {
struct Object;
}

namespace Slic3r::Biz::Slicing {
struct SLAResult;
}

namespace Slic3r::App::Scene {
class Node;
}

//#define USE_TEXTURE_BUFFER (1 && SLIC3R_RENDER_TEXTURE_BUFFER_SUPPORTED)

namespace Slic3r::App::libvgcode {

struct GCodeInputData;

class SlaViewer : public AbstractViewer
{
    using ModelGeometryManager = Render::GeometryManager<Scene::AuxiliaryElementId>;
    using ModelTriangleMeshManager = Scene::TriangleMeshManager<Scene::AuxiliaryElementId>;
public:
    SlaViewer();
    ~SlaViewer() = default;
    SlaViewer(const SlaViewer&) = delete;
    SlaViewer(SlaViewer&&) = delete;
    SlaViewer& operator = (const SlaViewer&) = delete;
    SlaViewer& operator = (SlaViewer&&) = delete;

    /**
     * @brief Initialize rendering geometry
     *
     * @param device The current device.
     * @param scene The current scene.
     * @param data_factory The geometry factory.
     */
    void init(Render::Device& device, Scene::Scene& scene, Scene::GeometryDataFactory& data_factory) override;
    //
    // Reset all caches and free gpu memory.
    //
    void reset() override;

    void load(const Biz::Slicing::SLAResult& result);
    void load_layers(const std::vector<float>& layers_zs, const std::vector<double>& layers_times);
    void load_object(const Biz::Slicing::Sla::Object& object);

    void reset_layers();
    void reset_object(const Domain::ObjectID object_id);
    //
    // Render
    //
    void render() override;

    void set_layers_range(Interval::value_type min, Interval::value_type max) override;
    void set_view_visible_range(Interval::value_type min, Interval::value_type max) override;

    float estimated_time() const override;
    float estimated_time_at(size_t id) const override;
    std::vector<float> layers_estimated_times() const override;

private:
    //
    // The OpenGL element used to represent all toolpath segments
    //

    size_t m_enabled_segments_count{ 0 };

    ModelGeometryManager m_model_geometry_manager;
    ModelTriangleMeshManager m_model_triangle_mesh_manager;

    Scene::Node* m_main_node{nullptr};
    const Biz::Slicing::SLAResult* m_result{ nullptr };

    Render::TextureBuffer* m_positions_buffer{ nullptr };
    Render::TextureBuffer* m_heights_widths_angles_buffer{ nullptr };
    Render::TextureBuffer* m_colors_buffer{ nullptr };
    Render::TextureBuffer* m_enabled_segments_buffer{ nullptr };

private:

    void build_instance_node(
        const Biz::Slicing::Sla::Object& sla_object, 
        size_t instance_id,
        const Domain::Transform3d& trafo,
        Scene::Node* parent_node);

    void build_if_needed(
        size_t object_id,
        size_t instance_id,
        SlaMeshType type,
        std::shared_ptr<const Slic3r::Domain::TriangleMesh> mesh,
        const Domain::Transform3d& trafo,
        Scene::Node* parent_node);

    void build_sla_object_mesh(
        size_t object_id,
        size_t instance_id,
        SlaMeshType type,
        std::shared_ptr<const Slic3r::Domain::TriangleMesh> mesh,
        const Domain::Transform3d& trafo,
        Scene::NodeBuilder& builder);

    void update_layer_preview_contour(const size_t layer_id);

    void update_view_full_range() override;
    void render_segments(const Domain::Vec3f& camera_position);
};

} // namespace Slic3r::App::libvgcode
