#include "Slic3r/App/libvgcode/SlaViewer.hpp"

#include <Slic3r/Biz/libpgcode/Utils.hpp>
#include <Slic3r/App/Render/GL/commonGL.hpp>
#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Render/Context.hpp>
#include <Slic3r/App/Render/TextureManager.hpp>
#include <Slic3r/App/Render/TextureBufferManager.hpp>
#include <Slic3r/App/Render/Material.hpp>
#include <Slic3r/App/Render/GeometryBuilder.hpp>
#include <Slic3r/App/Scene/NodeBuilder.hpp>
#include <Slic3r/App/Scene/Scene.hpp>
#include "Slic3r/App/Scene/InstancedMeshRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/ScenePresenterProjectContext.hpp"

#include "Slic3r/Domain/ObjectID.hpp"
#include "libslic3r/SLA/SLAResult.hpp"
#include "libslic3r/format.hpp"
#include "Slic3r/App/libvgcode/GCodeNodeTag.hpp"

#include <map>
#include <assert.h>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <memory>
#include <spdlog/spdlog.h>

using namespace Slic3r::Biz::libpgcode;
using Slic3r::Domain::Vec3f;
using Slic3r::Domain::ColorRGBA;
using Slic3r::Domain::Transform3d;
using Slic3r::Domain::ExPolygons;
using Slic3r::Domain::EPSILON;
using Slic3r::Domain::Vec2f;

namespace Slic3r::App::libvgcode {

SlaViewer::SlaViewer()
{
}

void SlaViewer::init(Render::Device& device, Scene::Scene& scene, Scene::GeometryDataFactory& data_factory)
{
    if (m_initialized)
        return;

    AbstractViewer::init(device, scene, data_factory);

    Scene::NodeBuilder builder{ *m_scene };
    builder.set_debug_name("sla_main");
    builder.set_tag(SlaObjectNodeTag());
    m_scene->add_child(builder.build().release(), &m_scene->root());
    m_main_node = m_scene->root().children().back().get();

    /* Create a tree
    * -> "main_sla"             {SlaObjectNodeTag{ object_id = 0, instance_id = 0, SlaMeshType::Undefined }}
    *       -> "sla_object_node"    {SlaObjectNodeTag{ object_id, instance_id = 0, SlaMeshType::Undefined }}
    *           -> "sla_instanceN_node" {SlaObjectNodeTag{ object_id, instance_id, SlaMeshType::Undefined }}
    *               -> "object_mesh"        {SlaObjectNodeTag{ object_id, instance_id, SlaMeshType::Object }}
    *               -> "supports_mesh"      {SlaObjectNodeTag{ object_id, instance_id, SlaMeshType::Supports }}
    *               -> "pad_mesh"           {SlaObjectNodeTag{ object_id, instance_id, SlaMeshType::Pad }}  
    * -> "segment node"
    */


    m_initialized = true;
}

void SlaViewer::reset()
{
    AbstractViewer::reset();

    // Reset other attributes, if needed

    if (m_positions_buffer != nullptr) m_positions_buffer->set_data(nullptr, 1, Render::BufferUsage::StaticDraw);
    if (m_heights_widths_angles_buffer != nullptr) m_heights_widths_angles_buffer->set_data(nullptr, 1, Render::BufferUsage::StaticDraw);
    if (m_colors_buffer != nullptr) m_colors_buffer->set_data(nullptr, 1, Render::BufferUsage::StaticDraw);
    if (m_enabled_segments_buffer != nullptr) m_enabled_segments_buffer->set_data(nullptr, 1, Render::BufferUsage::StaticDraw);

    m_enabled_segments_count = 0;
    m_result = nullptr;

    // remove all object Scene::Nodes 
    m_scene->remove_children([](const Scene::Node*) { return true; }, m_main_node);
    m_model_geometry_manager.release_all();
    m_model_triangle_mesh_manager.release_all();
}

void SlaViewer::reset_layers()
{
    AbstractViewer::reset();
}

void SlaViewer::reset_object(const Domain::ObjectID object_id)
{
    // remove object from the  Scene::Nodes 
    m_scene->remove_children([object_id](const Scene::Node* node) { 
        const SlaObjectNodeTag* t = node->tag_of_type<SlaObjectNodeTag>();
        return t && t->object_id == object_id.id;
    }, m_main_node);

    for (const Scene::AuxiliaryElementId::Type& aei_type : { Scene::AuxiliaryElementId::Type::SlaMesh,
        Scene::AuxiliaryElementId::Type::SlaSupports,
        Scene::AuxiliaryElementId::Type::SlaPad }) {

        Scene::AuxiliaryElementId id{ aei_type, object_id.id };

        m_model_geometry_manager.release(id);
        m_model_triangle_mesh_manager.release(id);
    }
}

void SlaViewer::load(const Biz::Slicing::SLAResult& result)
{
    if (result.print_statistics.has_value()) {
        load_layers(result.heights, result.print_statistics.value().layers_times_running_total);
        m_result = &result;
    }
}

void SlaViewer::load_layers(const std::vector<float>& layers_zs, const std::vector<double>& layers_times)
{
    if (!m_initialized)
        return;

    if (layers_zs.empty() || layers_times.empty())
        return;

    AbstractViewer::reset();

    assert(layers_zs.size() == layers_times.size());

    for (size_t i = 0; i < layers_zs.size(); ++i) {
        m_layers.update_as_sla(layers_zs[i], layers_times[i]);
    }
}

static const std::unordered_map<SlaMeshType, ColorRGBA> SLA_MESH_COLORS = {
    {SlaMeshType::Object, {1, 0.5f, 0, 1}},
    {SlaMeshType::Supports, {0.5f, 0.5f, 0.5f, 1}},
    {SlaMeshType::Pad, {0.6f, 0.2f, 1.0f, 1}},
};

void SlaViewer::build_sla_object_mesh(
    size_t object_id,
    size_t instance_id,
    SlaMeshType type, 
    std::shared_ptr<const Slic3r::Domain::TriangleMesh> mesh,
    const Transform3d& trafo,
    Scene::NodeBuilder& builder)
{
    const std::string type_str = type == SlaMeshType::Object   ? "object" :
                                 type == SlaMeshType::Supports ? "support" :
                                 type == SlaMeshType::Pad      ? "pad" : "UNDEF";
    SPDLOG_DEBUG("build_volume obj:{} inst:{} type:{}", object_id, instance_id, type_str);

    auto& geom_mgr = m_model_geometry_manager;
    auto& trimesh_mgr = m_model_triangle_mesh_manager;

    Scene::AuxiliaryElementId::Type aei_type =  type == SlaMeshType::Object   ? Scene::AuxiliaryElementId::Type::SlaMesh :
                                                type == SlaMeshType::Supports ? Scene::AuxiliaryElementId::Type::SlaSupports :
                                                type == SlaMeshType::Pad      ? Scene::AuxiliaryElementId::Type::SlaPad : 
                                                                                Scene::AuxiliaryElementId::Type::Volume;

    Scene::AuxiliaryElementId id{ aei_type, object_id};
    const auto& trimesh =
        trimesh_mgr.get_or_create(id, [&]() -> std::unique_ptr<Scene::TriangleMesh> {
        return std::make_unique<Scene::TriangleMesh>(mesh);
            });
    const auto* geom = geom_mgr.get_or_create(id, [&]() {
        return Render::geometry_from_triangle_mesh(*m_device, trimesh->triangles());
        });
    ColorRGBA color = ColorRGBA{ 1.0f, 1.0f, 1.0f, 1.0f };

    auto color_it = SLA_MESH_COLORS.find(type);
    if (color_it != SLA_MESH_COLORS.end())
        color = color_it->second;

    auto material = Render::Material{}
        .set_shader(m_device->context().shader_manager().shader("gouraud_light"))
        .set_uniform("uniform_color", color)
        .set_transparent(color.is_transparent());

    builder
        .set_debug_name(Slic3r::format("sla_obj: %1% : %2%", object_id, type_str))
        .set_tag(SlaObjectNodeTag{ object_id, instance_id, type })
        .set_mesh(geom, material, int(0))
        .transform([trafo](auto& xform) { xform = trafo; })
        .set_aabb(trimesh->aabb_mesh())
        .set_shadows(Render::Shadows{ true, true });
}


void SlaViewer::build_if_needed(
    size_t object_id,
    size_t instance_id,
    SlaMeshType type,
    std::shared_ptr<const Slic3r::Domain::TriangleMesh> mesh,
    const Transform3d& trafo, 
    Scene::Node* parent_node)
{
    if (!mesh || mesh->empty()) {
        return;
    }

    for (auto& node : parent_node->children()) {
        const SlaObjectNodeTag* t = node->tag_of_type<SlaObjectNodeTag>();
        ASSERT(t && t->object_id == object_id && t->instance_id == instance_id);
        if (t->type == type) {
            // node for this mesh type already exists
            return;
        }
    }

    Scene::NodeBuilder builder(*m_scene);

    build_sla_object_mesh(object_id, instance_id, type, mesh, trafo, builder);

    m_scene->add_child(builder.build().release(), parent_node);
}

void SlaViewer::build_instance_node(
    const Biz::Slicing::Sla::Object& sla_object,
    size_t instance_id,
    const Transform3d& trafo,
    Scene::Node* parent_node)
{
    // add Scene::Nodes for each mesh from object
    Scene::NodeBuilder builder(*m_scene);

    size_t object_id = sla_object.object_id.id;

    build_if_needed(object_id, instance_id, SlaMeshType::Object,   sla_object.mesh,              trafo, parent_node);
    build_if_needed(object_id, instance_id, SlaMeshType::Supports, sla_object.support_structure, trafo, parent_node);
    build_if_needed(object_id, instance_id, SlaMeshType::Pad,      sla_object.pad,               trafo, parent_node);

    for (const auto& node : parent_node->children()) {
        const SlaObjectNodeTag* t = node->tag_of_type<SlaObjectNodeTag>();
        if (t != nullptr && t->instance_id == instance_id) {
            node->set_local_transform(trafo.matrix());
        }
    }
}

void SlaViewer::load_object(const Biz::Slicing::Sla::Object& sla_object)
{
    size_t object_id = sla_object.object_id.id;

    Scene::Node* object_node{ nullptr };
    for (auto& node : m_main_node->children()) {
        const SlaObjectNodeTag* t = node->tag_of_type<SlaObjectNodeTag>();
        if (t != nullptr && t->object_id == object_id) {
            // "sla_object" node already exists 
            ASSERT(t->type == SlaMeshType::Undefined);
            object_node = node.get();
            break;
        }
    }

    if (!object_node) {
        // Create object node if it wasn't found
        Scene::NodeBuilder builder{ *m_scene };
        builder.set_debug_name(Slic3r::format("sla_object: %1%", object_id));
        builder.set_tag(SlaObjectNodeTag({ object_id }));
        m_scene->add_child(builder.build().release(), m_main_node);
        object_node = m_main_node->children().back().get();
    }
    ASSERT(object_node);

    // add instances nodes
    for (const auto& [id, trafo] : sla_object.instance_trafos) {
        size_t instance_id = id.id;
        Scene::Node* inst_node{ nullptr };
        for (auto& node : object_node->children()) {
            const SlaObjectNodeTag* t = node->tag_of_type<SlaObjectNodeTag>();
            ASSERT(t->object_id == object_id && t->type == SlaMeshType::Undefined);
            if (t != nullptr && t->instance_id == instance_id) {
                // "instance" node already exists
                inst_node = node.get();
                break;
            }
        }

        if (!inst_node) {
            // Create instance node if it wasn't found
            Scene::NodeBuilder builder{ *m_scene };
            builder.set_debug_name(Slic3r::format("sla_object: %1%, instance %2%", object_id, instance_id));
            builder.set_tag(SlaObjectNodeTag({ object_id, instance_id }));
            m_scene->add_child(builder.build().release(), object_node);
            inst_node = object_node->children().back().get();
        }
        ASSERT(inst_node);

        // add mesh node
        build_instance_node(sla_object, instance_id, trafo, inst_node);
    }
}

void SlaViewer::render()
{
    if (m_layers.empty())
        return;

    render_segments(m_scene->camera().position().cast<float>());
}

static Vec3f get_vec3(const Slic3r::Domain::Point& v, const float z)
{
    return { static_cast<float>(Slic3r::Domain::SCALING_FACTOR * v.x()), static_cast<float>(Slic3r::Domain::SCALING_FACTOR * v.y()), z };
}

static void extract_pos_and_or_hwa(const Slic3r::Domain::ExPolygon& vertices, float z, float height,
    std::vector<Vec4f>& positions, std::vector<Vec4f>& heights_widths_angles)
{
    if (vertices.empty())
        return;

    const Slic3r::Domain::Polygon& contour = vertices.contour;
    size_t contour_size = contour.size();

    const float width{ 0.5f };

    Vec4f pos_first = Vec4f::Zero();
    Vec4f hwa_first = Vec4f::Zero();

    for (size_t i = 0; i < contour_size; ++i) {
        const Slic3r::Domain::Point& v = contour.points[i];
        const Slic3r::Domain::Point& v_prev = contour.points[i > 0 ? i - 1 : contour_size - 1];
        const Slic3r::Domain::Point& v_next = contour.points[i + 1 < contour_size ? i + 1 : 0];

        Vec3f pos = get_vec3(v, z);
        Vec3f pos_prev = get_vec3(v_prev, z);
        Vec3f pos_next = get_vec3(v_next, z);

        Vec3f prev_line = pos - pos_prev;
        Vec3f this_line = pos_next - pos;

        Vec4f position = { pos.x(), pos.y(), pos.z(), 0.0f };
        // the last component is a dummy float to comply with GL_RGBA32F format
        position.z() -= 0.5f * height;
        positions.emplace_back(position);

        if (i == 0) {
            // Add 'phantom' position and zero heights_widths_angle
            heights_widths_angles.push_back(Vec4f::Zero());
            positions.emplace_back(position);
        }

        // the last component is a dummy float to comply with GL_RGBA32F format
        heights_widths_angles.push_back({ height, width,
            std::atan2(prev_line.x() * this_line.y() - prev_line.y() * this_line.x(), prev_line.dot(this_line)), 0.0f });

        if (i == 0) {
            pos_first = position;
            hwa_first = heights_widths_angles.back();
        }
    }

    // Add 'phantom' position and heights_widths_angle of the first vertex of the contoure
    positions.emplace_back(pos_first);
    heights_widths_angles.push_back(hwa_first);

    // Add 'phantom' position and zero heights_widths_angle
    positions.emplace_back(pos_first);
    heights_widths_angles.push_back(Vec4f::Zero());
}

void SlaViewer::update_layer_preview_contour(const size_t layer_id)
{
    if (m_layers.empty() || !m_result) {
        return;
    }

    float color = encoded_color({ 0.5f, 0.7f, 0.3f });
    m_layers.set_view_range(0, uint32_t(m_layers.count()) - 1);

    const float layer_z = m_result->heights[layer_id];
    const float layer_height = layer_id == 0 ? m_result->heights[layer_id] : m_result->heights[layer_id]-m_result->heights[layer_id-1];

    const ExPolygons& layer_polygons = m_result->slices[layer_id];

    // For each polygon we need to add 3 'phantom' positions and heights_widths_angles to properly preview of it's start/end.
    size_t vertex_cnt{ 3 * layer_polygons.size() };

    for (const auto& polygon : layer_polygons)
        vertex_cnt += polygon.contour.size();

    // buffers to send to gpu
    // the last component is a dummy float to comply with GL_RGBA32F format
    std::vector<Vec4f> positions;
    std::vector<Vec4f> heights_widths_angles;
    positions.reserve(vertex_cnt);
    heights_widths_angles.reserve(vertex_cnt);

    for (const auto& polygon : layer_polygons) {
        extract_pos_and_or_hwa(polygon, layer_z, layer_height, positions, heights_widths_angles);
    }

    if (!positions.empty()) {

        // create and fill positions buffer
        m_positions_buffer = m_device->context().texture_buffer_manager().get_or_create_empty("gcode_positions", Render::PixelFormat::RGBA32F);
        m_positions_buffer->set_data(positions.data(), positions.size() * sizeof(Vec4f), Render::BufferUsage::StaticDraw);

        // create and fill height, width and angles buffer
        m_heights_widths_angles_buffer = m_device->context().texture_buffer_manager().get_or_create_empty("gcode_heights_widths_angles", Render::PixelFormat::RGBA32F);
        m_heights_widths_angles_buffer->set_data(heights_widths_angles.data(), heights_widths_angles.size() * sizeof(Vec4f), Render::BufferUsage::DynamicDraw);

        // create (but do not fill) colors buffer (data is set in update_colors())
        m_colors_buffer = m_device->context().texture_buffer_manager().get_or_create_empty("gcode_colors", Render::PixelFormat::R32F);
        // Based on current settings and slider position, we might want to render some
        // vertices as dark grey. Use either that or the normal color (from the cache).
        std::vector<float> colors(vertex_cnt, color);
        m_colors_buffer->set_data(colors.data(), colors.size() * sizeof(float), Render::BufferUsage::StaticDraw);

        // create (but do not fill) enabled segments buffer (data is set in update_enabled_entities())
        m_enabled_segments_buffer = m_device->context().texture_buffer_manager().get_or_create_empty("gcode_enabled_segments", Render::PixelFormat::R32UI);

        std::vector<uint32_t> enabled_segments;
        for (size_t i = 0; i < positions.size(); ++i) {
            enabled_segments.push_back(uint32_t(i));
        }
        m_enabled_segments_count = enabled_segments.size();

        // update buffer for enabled segments
        assert(m_enabled_segments_buffer != nullptr);
        m_enabled_segments_buffer->set_data(enabled_segments.data(), enabled_segments.size() * sizeof(uint32_t), Render::BufferUsage::StaticDraw);
    }
}

void SlaViewer::set_layers_range(Interval::value_type min, Interval::value_type max)
{
    AbstractViewer::set_layers_range(min, max);
    update_layer_preview_contour(max);
}

void SlaViewer::set_view_visible_range(Interval::value_type min, Interval::value_type max)
{
    AbstractViewer::set_view_visible_range(min, max);
    update_layer_preview_contour(max);    
}

void SlaViewer::update_view_full_range()
{

}

float SlaViewer::estimated_time() const
{
    return 0.f;
}

float SlaViewer::estimated_time_at(size_t id) const
{
    return 0.f;
}

std::vector<float> SlaViewer::layers_estimated_times() const
{
    return m_layers.times(Biz::libpgcode::TimeMode::Normal);
}

static constexpr int POSITION_TEX_ID = 0;
static constexpr int HEIGHT_WIDTH_ANGLE_TEX_ID = 1;
static constexpr int COLOR_TEX_ID = 2;
static constexpr int ENABLED_SEGMENTS_TEX_ID = 3;
static constexpr int ENABLED_OPTIONS_TEX_ID = 3;

void SlaViewer::render_segments(const Vec3f& camera_position)
{
    Scene::Node* node = m_scene->root().query_first([](const Scene::Node* n)->bool {
        const GCodeNodeTag* tag = n->tag_of_type<GCodeNodeTag>();
        return tag != nullptr && tag->type == GCodeElementType::Toolpaths;
        }, true);

    assert(node != nullptr);
    node->set_enabled(m_enabled_segments_count > 0);

    if (m_enabled_segments_count == 0)
        return;

    Render::Material material{};
    material
        .set_shader(m_device->context().shader_manager().shader("segments"));
    Scene::set_uniforms(m_lights, material);

    material
        .set_uniform("position_tex", POSITION_TEX_ID)
        .set_uniform("height_width_angle_tex", HEIGHT_WIDTH_ANGLE_TEX_ID)
        .set_uniform("color_tex", COLOR_TEX_ID)
        .set_uniform("segment_index_tex", ENABLED_SEGMENTS_TEX_ID)
        .set_uniform("camera_position", camera_position)
        .set_texture_buffer(POSITION_TEX_ID, m_positions_buffer)
        .set_texture_buffer(HEIGHT_WIDTH_ANGLE_TEX_ID, m_heights_widths_angles_buffer)
        .set_texture_buffer(COLOR_TEX_ID, m_colors_buffer)
        .set_texture_buffer(ENABLED_SEGMENTS_TEX_ID, m_enabled_segments_buffer);

    node->set_material_override(material);
    Scene::InstancedMeshRenderNodeComponent* r_comp = dynamic_cast<Scene::InstancedMeshRenderNodeComponent*>(node->render_component());
    r_comp->set_instances_count(m_enabled_segments_count);
}

} // namespace Slic3r::App::libvgcode
