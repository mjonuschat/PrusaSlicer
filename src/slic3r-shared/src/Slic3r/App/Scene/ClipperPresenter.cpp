#include "Slic3r/App/Scene/ClipperPresenter.hpp"

#include "Slic3r/App/Scene/Clipper.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/Node.hpp"
#include <Slic3r/App/Scene/NodeBuilder.hpp>
#include <Slic3r/App/Scene/NodeVisitor.hpp>
#include "Slic3r/App/Scene/MeshRenderNodeComponent.hpp"

#include "Slic3r/Domain/ModelVolume.hpp"

#include "Slic3r/App/Render/Device.hpp"
#include <Slic3r/App/Render/GeometryBuilder.hpp>

#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include <fmt/format.h>

using Slic3r::Domain::ColorRGBA;

namespace Slic3r::App::Scene {

ClipperPresenter::ClipperPresenter(Clipper* clipper, Render::Device* device) :
    m_clipper(clipper),
    m_device(device)
{}

static void set_enabled_scene_nodes(
    Scene* scene,
    bool enabled_scene_nodes,
    Node* main_presenter_node,
    bool force_enabled_scene_nodes = true
)
{
    if (force_enabled_scene_nodes) {
        visit(
            scene->root(),
            [&](Node& node)
            {
                Plater::SceneNodeTag* tag = node.tag_of_type<Plater::SceneNodeTag>();
                if (tag != nullptr) {
                    node.set_enabled(enabled_scene_nodes);
                }
            },
            true
        );
    }
    if (main_presenter_node) {
        main_presenter_node->set_enabled(!enabled_scene_nodes);
    }
}

void ClipperPresenter::activate(
    Scene* scene,
    const Domain::ModelObject* selected_object,
    const Domain::ModelInstance* selected_instance,
    double sla_shift
)
{
    if (m_main_node && m_main_node->children().size() > 0) {
        deactivate();
    }

    m_scene = scene;
    if (m_clipper) {
        m_clipper->set_camera(&m_scene->camera());
        m_clipper->update(selected_object, selected_instance, sla_shift, true);
    }
    init_main_node();
    build_meshes_nodes(selected_instance->get_matrix());
    set_enabled_scene_nodes(m_scene, false, m_main_node);
}

void ClipperPresenter::deactivate(bool force_enabled_scene_nodes /*= true*/)
{
    reset();
    set_enabled_scene_nodes(m_scene, true, m_main_node, force_enabled_scene_nodes);
}

void ClipperPresenter::reset()
{
    if (m_main_node) {
        m_scene->remove_children(
            [&](const Node* node)
            {
                const ClipperElement* tag = node->tag_of_type<ClipperElement>();
                return tag->type != ClipperElementType::Undef;
            },
            m_main_node
        );

        m_model_geometry_manager.release_all();
        m_model_triangle_mesh_manager.release_all();
    }
}

void ClipperPresenter::init_main_node()
{
    Node* node = m_scene->root().query_first(
        [](const Node* node) -> bool
        {
            const ClipperElement* tag = node->tag_of_type<ClipperElement>();
            return tag != nullptr;
        },
        true
    );
    if (node) {
        m_main_node = node;
        return;
    }
    NodeBuilder builder{*m_scene};
    builder.set_debug_name("Clipper main").set_tag(ClipperElement());

    m_scene->add_child(builder.build().release(), &m_scene->root());
    m_main_node = m_scene->root().children().back().get();
}

void ClipperPresenter::build_meshes_nodes(const Domain::Transform3d& inst_trafo)
{
    const size_t volumes_cnt = m_clipper->volumes().size();
    for (size_t volume_id = 0; volume_id < volumes_cnt; volume_id++) {
        const Domain::ModelVolume* volume = m_clipper->volumes()[volume_id];

        ClipperElement id{ClipperElementType::Mesh, volume_id};

        const auto& trimesh = m_model_triangle_mesh_manager.get_or_create(
            id,
            [&]() -> std::unique_ptr<TriangleMesh>
            { return std::make_unique<TriangleMesh>(volume->mesh_ptr()); }
        );
        const auto* geom = m_model_geometry_manager.get_or_create(
            id,
            [&]() { return Render::geometry_from_triangle_mesh(*m_device, trimesh->triangles()); }
        );

        auto trafo = inst_trafo * volume->get_matrix();

        ColorRGBA color = m_mesh_color;
        if (!volume->is_model_part())
            color.set(3, 0.75f);
        auto material =
            Render::Material{}
                .set_shader(m_device->context().shader_manager().shader("gouraud_light_clip"))
                .set_uniform("uniform_color", color)
                .set_transparent(color.is_transparent());

        NodeBuilder builder{*m_scene};
        builder.set_debug_name(fmt::format("Clipped model:vol {}", volume_id))
            .set_tag(id)
            .set_mesh(geom, material, int(0))
            .transform([trafo, volume](auto& xform) { xform = trafo; });

        m_scene->add_child(builder.build().release(), m_main_node);
    }
}

Domain::Vec4f get_clipping_plane_data(const Clipper* clipper)
{
    Domain::Vec4f clp_data_out(0.f, 0.f, 1.f, FLT_MAX);
    // Take care of the clipping plane. The normal of the clipping plane is
    // saved with opposite sign than we need to pass to OpenGL (FIXME)
    if (bool clipping_plane_active = clipper->get_position() != 0.; clipping_plane_active) {
        const Biz::ClippingPlane* clp = clipper->get_clipping_plane();
        for (size_t i = 0; i < 3; ++i)
            clp_data_out[i] = -1.f * float(clp->get_data()[i]);
        clp_data_out[3] = float(clp->get_data()[3]);
    }

    return clp_data_out;
}

void ClipperPresenter::build_non_mesh_node(
    ClipperElementType type,
    const indexed_triangle_set& its,
    size_t clipper_id,
    size_t island_id
)
{
    ASSERT(type == ClipperElementType::Plane || type == ClipperElementType::Contour);
    ClipperElement id{type, clipper_id, island_id};

    indexed_triangle_set mesh_its = its;
    const auto& trimesh           = m_model_triangle_mesh_manager.get_or_create(
        id,
        [&]() -> std::unique_ptr<TriangleMesh>
        { return std::make_unique<TriangleMesh>(std::move(mesh_its)); }
    );
    const auto* geom = m_model_geometry_manager.get_or_create(
        id,
        [&]() { return Render::geometry_from_triangle_mesh(*m_device, trimesh->triangles()); }
    );

    const bool is_palne = type == ClipperElementType::Plane;
    auto material =
        Render::Material{}
            .set_shader(m_device->context().shader_manager().shader(/*"gouraud_light"*/ "flat"))
            .set_uniform("uniform_color", is_palne ? m_plane_color : m_contour_color);

    NodeBuilder bldr(*m_scene);
    bldr.set_debug_name(
            fmt::format(
                "Clipped {}:id {}, island {}",
                is_palne ? "plane" : "contour",
                clipper_id,
                island_id
            )
    )
        .set_tag(id)
        .set_mesh(geom, material, int(0));

    if (is_palne) {
        bldr.set_aabb(trimesh->aabb_mesh());
    }

    m_scene->add_child(bldr.build().release(), m_main_node);
}

void ClipperPresenter::update_nodes()
{
    // update clipping plane data for meshes
    visit(
        *m_main_node,
        [&](Node& node)
        {
            ClipperElement* tag   = node.tag_of_type<ClipperElement>();
            auto render_component = static_cast<MeshRenderNodeComponent*>(node.render_component());

            if (tag->type == ClipperElementType::Mesh) {
                Render::Material material = render_component->material();
                material.set_uniform("clipping_plane", get_clipping_plane_data(m_clipper));
                node.set_material_override(material);
                return;
            }
        },
        true
    );

    // remove Plane and Contour nodes

    m_scene->remove_children(
        [&](const Node* node)
        {
            const ClipperElement* tag = node->tag_of_type<ClipperElement>();
            if (tag->type == ClipperElementType::Plane || tag->type == ClipperElementType::Contour)
            {
                m_model_triangle_mesh_manager.release(*tag);
                m_model_geometry_manager.release(*tag);
                return true;
            }
            return false;
        },
        m_main_node
    );

    // Build new Plane and Contour nodes

    size_t clipper_id = 0;
    for (const auto& [mesh_clipper, trafo] : m_clipper->object_clippers) {
        if (mesh_clipper->result) {
            size_t island_id = 0;
            for (const Biz::MeshClipper::CutIsland& island :
                 mesh_clipper->result.value().cut_islands)
            {
                if (std::find(
                        m_ignored_ids.begin(),
                        m_ignored_ids.end(),
                        ClipperId{clipper_id, island_id}
                    )
                    == m_ignored_ids.end())
                {
                    // Add Plane
                    build_non_mesh_node(
                        ClipperElementType::Plane,
                        island.model,
                        clipper_id,
                        island_id
                    );

                    // Add Contour
                    build_non_mesh_node(
                        ClipperElementType::Contour,
                        island.model_expanded,
                        clipper_id,
                        island_id
                    );
                }

                island_id++;
            }
            clipper_id++;
        }
    }
}

void ClipperPresenter::set_clickable_plane(bool clickable) {}

static void set_node_enabled(Node* parent, ClipperElementType node_type, bool enabled)
{
    visit(
        *parent,
        [&](Node& node)
        {
            ClipperElement* tag = node.tag_of_type<ClipperElement>();
            if (tag->type == node_type) {
                node.set_enabled(enabled);
                return;
            }
        },
        true
    );
}

static void set_node_color(Node* parent, ClipperElementType node_type, ColorRGBA color)
{
    visit(
        *parent,
        [&](Node& node)
        {
            ClipperElement* tag = node.tag_of_type<ClipperElement>();
            if (tag->type == node_type) {
                Render::Material material = node.render_component()->material();
                material.set_uniform("uniform_color", color);
                node.set_material_override(material);
                return;
            }
        },
        true
    );
}

void ClipperPresenter::set_enable_mesh(bool enable)
{
    if (m_mesh_enabled != enable) {
        m_mesh_enabled = enable;
        set_node_enabled(m_main_node, ClipperElementType::Mesh, m_mesh_enabled);
    }
}

void ClipperPresenter::set_enable_plane(bool enable)
{
    if (m_plane_enabled != enable) {
        m_plane_enabled = enable;
        set_node_enabled(m_main_node, ClipperElementType::Plane, m_plane_enabled);
    }
}

void ClipperPresenter::set_enable_contour(bool enable)
{
    if (m_contour_enabled != enable) {
        m_contour_enabled = enable;
        set_node_enabled(m_main_node, ClipperElementType::Contour, m_contour_enabled);
    }
}

void ClipperPresenter::set_color_mesh(Slic3r::Domain::ColorRGBA color)
{
    if (m_mesh_color != color) {
        m_mesh_color = color;
        set_node_color(m_main_node, ClipperElementType::Mesh, m_mesh_color);
    }
}

void ClipperPresenter::set_color_plane(Slic3r::Domain::ColorRGBA color)
{
    if (m_plane_color != color) {
        m_plane_color = color;
        set_node_color(m_main_node, ClipperElementType::Plane, m_plane_color);
    }
}

void ClipperPresenter::set_color_contour(Slic3r::Domain::ColorRGBA color)
{
    if (m_contour_color != color) {
        m_contour_color = color;
        set_node_color(m_main_node, ClipperElementType::Contour, m_contour_color);
    }
}

void ClipperPresenter::reset_ignored()
{
    m_ignored_ids.clear();
}

void ClipperPresenter::add_ignored(size_t volume_id, size_t island_id)
{
    m_ignored_ids.emplace_back(ClipperId{volume_id, island_id});
}

void ClipperPresenter::show_clipper(bool show) {}

void ClipperPresenter::set_position_by_ratio(double pos, bool keep_normal)
{
    if (m_clipper) {
        m_clipper->set_position_by_ratio(pos, keep_normal);
        reset_ignored();
        update_nodes();
    }
}

void
ClipperPresenter::set_range_and_pos(const Domain::Vec3d& cpl_normal, double cpl_offset, double pos)
{
    if (m_clipper) {
        m_clipper->set_range_and_pos(cpl_normal, cpl_offset, pos);
        reset_ignored();
        update_nodes();
    }
}

void ClipperPresenter::set_behavior(bool hide_clipped, bool fill_cut, double contour_width)
{
    if (m_clipper) {
        m_clipper->set_behavior(hide_clipped, fill_cut, contour_width);
        update_nodes();
    }
}

int ClipperPresenter::is_projection_inside_cut(const Domain::Vec3d& point_in) const
{
    if (m_clipper) {
        return m_clipper->is_projection_inside_cut(point_in);
    }
    return -1;
}


GizmoActivationState ClipperPresenter::on_mouse(GizmoEventContext& ctx, bool only_active)
{
    const auto event_type = ctx.mouse_event().type();
    if (event_type != Platform::MouseEvent::Type::ButtonDown
        && event_type != Platform::MouseEvent::Type::Move
        && event_type != Platform::MouseEvent::Type::ButtonUp)
    {
        return GizmoActivationState::Inactive;
    }

    const auto& pick_ray = ctx.pick_ray();
    if (event_type == Platform::MouseEvent::Type::ButtonDown) {
        if (const Node* handle_node =
            ctx.pick_result_node_with_tag_of_type<ClipperElement>())
        {
            const ClipperElement& tag = *handle_node->tag_of_type<ClipperElement>();
            ASSERT(tag.type == ClipperElementType::Plane);

            return GizmoActivationState::Active;
        }
        else {
            return GizmoActivationState::Inactive;
        }

        //const App::Scene::Transform& xform = m_scene_presenter.selection_root().world_transform();
        //m_translation_ray.origin = xform.matrix().block<3, 1>(0, 3);
    }

    //double t;
    //if (!m_translation_ray.closest_point_from_ray(pick_ray, t)) {
    //    return GizmoActivationState::Inactive;
    //}

    if (event_type == Platform::MouseEvent::Type::ButtonDown)
    {
        return GizmoActivationState::Active;
    }
/*
    if (event_type == Platform::MouseEvent::Type::ButtonUp) {
        // fill clicked position 
        return GizmoActivationState::Done;
    }*/

    return GizmoActivationState();
}

} // namespace Slic3r::App::Scene
