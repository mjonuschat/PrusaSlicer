#include "Slic3r/App/Plater/GizmoDataFactory.hpp"
#include "Slic3r/App/Plater/PlaterSceneLayer.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"

namespace Slic3r::App::Plater {

namespace {

constexpr double TWO_PI = 2.0 * PI;
constexpr double HALF_PI = 0.5 * PI;

constexpr double AXIS_LINE_LENGTH = 30.0;
constexpr double CONE_RADIUS = 5.0;
constexpr double CONE_HEIGHT = 10.0;
constexpr double ANGLE_STEP = 2 * PI / 32; // 32 steps in full circle

constexpr uint8_t CIRCLE_RES = 64; // 64 steps in full circle
constexpr float CIRCLE_ANGLE_STEP = float(TWO_PI) / CIRCLE_RES;

void orient_object_pointing_to_positive_z(
    GizmoDataTransform data_transform, Transform3d& xform, const Vec3d& pre_translate = Vec3d::Zero()
)
{
    // an object points in +Z direction,
    constexpr float right_angle = float(HALF_PI);
    switch (data_transform) {
    case GizmoDataTransform::PointX:
        // make it pointing in +X direction
        xform.rotate(Eigen::AngleAxisd{right_angle, Vec3d::UnitY()});
        break;

    case GizmoDataTransform::PointY:
        // make it pointing in +Y direction
        xform.rotate(Eigen::AngleAxisd{-right_angle, Vec3d::UnitX()});
        break;

    case GizmoDataTransform::PointZ:
    case GizmoDataTransform::None:
        // no special handling needed
        break;
    }
    xform.translate(pre_translate);

}

} // namespace


void GizmoDataFactory::create_data(GizmoDataId id)
{
    indexed_triangle_set its;

    switch (id) {
    case GizmoDataId::ConeHandle:
        its = its_make_cone(CONE_RADIUS, CONE_HEIGHT, ANGLE_STEP);
        break;

    case GizmoDataId::AxesLines:
        create_axes_lines();
        return;

    case GizmoDataId::Segment: 
        // creates a segment 1 unit long
        create_segment();
        return;

    case GizmoDataId::Cone:
        // creates a cone contained into a 1x1x1 box 
        its = its_make_cone(0.5, 1.0, ANGLE_STEP);
        break;

    case GizmoDataId::Cube:
        // creates a 1x1x1 cube 
        its = its_make_cube(1.0, 1.0, 1.0);
        its_translate(its, -0.5f * Vec3f::Ones());
        break;

    case GizmoDataId::Circle: 
        // creates a circle with diameter equal to 1 
        create_circle();
        return;

    case GizmoDataId::GradedCircle: 
        // creates a graded circle with diameter equal to 1 
        create_graded_circle();
        return;

    default:
        UNREACHABLE("Invalid GizmoDataId", id);
    }

    Render::Material material;
    material.set_shader(m_device.context().shader_manager().get_shader("gouraud_light"));
    m_geometry_manager.set(id, Render::geometry_from_triangle_mesh(m_device, its, material));
    m_triangle_mesh_manager.set(id, std::make_unique<Scene::TriangleMesh>(std::move(its)));
}

std::unique_ptr<Scene::Node> GizmoDataFactory::create_node(
    Scene::Scene& scene,
    GizmoDataId data_id,
    GizmoDataVariant data_variant,
    GizmoDataTransform data_transform
)
{
    auto geom = geometry(data_id);
    auto trimesh = triangle_mesh(data_id);
    Scene::NodeBuilder builder{scene};

    // based on data_variant choose the material
    std::optional<ColorRGBA> color;

    switch (data_variant) {
    case GizmoDataVariant::Red:
        color = ColorRGBA{1, 0.2f, 0.2f, 1};
        break;

    case GizmoDataVariant::Green:
        color = ColorRGBA{0.2f, 1, 0.2f, 1};
        break;

    case GizmoDataVariant::Blue:
        color = ColorRGBA{0.2f, 0.2f, 1, 1};
        break;

    case GizmoDataVariant::None:
        // no color intentionaly set
        break;
    }

    Render::Material mat;

    if (color.has_value())
        mat.set_uniform("uniform_color", *color);

    builder
            .transform([data_id, data_transform](auto& xform) {
                switch (data_id) {
                case GizmoDataId::ConeHandle:
                    orient_object_pointing_to_positive_z(data_transform, xform, {0, 0, AXIS_LINE_LENGTH});
                    break;

                default:
                    DEBUG_ASSERT(
                        data_transform == GizmoDataTransform::None, "Unsupported gizmo transform",
                        std::make_pair(data_id, data_transform)
                    );
                    break;
                }
            })
            .set_mesh(geom, mat, int(PlaterSceneLayer::GizmoHandles));

    if (trimesh)
        builder.set_aabb(trimesh->aabb_mesh());

    return builder.build();
}

void GizmoDataFactory::create_axes_lines()
{
    Render::GeometryBuilder<Render::VertexP3> builder;
    Render::Material material = Render::Material{}
        .set_shader(m_device.context().shader_manager().get_shader("gouraud_light"))
        .set_uniform("uniform_color", ColorRGBA{0.2f, 0.2f, 0.2f, 1.0f});
    builder
        .add_vertex({{0, 0, 0}})
        .add_vertex({{AXIS_LINE_LENGTH, 0, 0}})
        .add_vertex({{0, AXIS_LINE_LENGTH, 0}})
        .add_vertex({{0, 0, AXIS_LINE_LENGTH}})
        .add_indices({0, 1, 0, 2, 0, 3, 0, 4})
        .add_draw_command({Render::PrimitiveType::Lines, 0, 6,  material});
    m_geometry_manager.set(GizmoDataId::AxesLines, builder.build(m_device));

}

void GizmoDataFactory::create_segment()
{
    Render::GeometryBuilder<Render::VertexP3> builder;
    builder
        .add_vertex({{ 0.0f, 0.0f, 0.0f }})
        .add_vertex({{ 1.0f, 0.0f, 0.0f }})
        .add_draw_command({Render::PrimitiveType::Lines, 0, 2,  Render::Material{} });
    m_geometry_manager.set(GizmoDataId::Segment, builder.build(m_device));
}

void GizmoDataFactory::create_circle()
{
    Render::GeometryBuilder<Render::VertexP3> builder;
    for (uint8_t i = 0; i < CIRCLE_RES; ++i) {
        float angle = i * CIRCLE_ANGLE_STEP;
        builder.add_vertex({{ 0.5f * cos(angle), 0.5f * sin(angle), 0.0f }});
    }
    builder
        .add_draw_command({ Render::PrimitiveType::LineLoop, 0, CIRCLE_RES,  Render::Material{} });
    m_geometry_manager.set(GizmoDataId::Circle, builder.build(m_device));
}

void GizmoDataFactory::create_graded_circle()
{
    Render::GeometryBuilder<Render::VertexP3> builder;
    for (uint8_t i = 0; i < CIRCLE_RES; ++i) {
        float angle_i = i * CIRCLE_ANGLE_STEP;
        float angle_j = (i + 1) * CIRCLE_ANGLE_STEP;
        builder.add_vertex({ { 0.5f * cos(angle_i), 0.5f * sin(angle_i), 0.0f } });
        builder.add_vertex({ { 0.5f * cos(angle_j), 0.5f * sin(angle_j), 0.0f } });
    }
    size_t vertices_count = 2 * CIRCLE_RES;
    float grade_step = float(TWO_PI) / CIRCLE_FINE_GRADE_SECONDARY_STEPS;
    for (uint8_t i = 0; i < CIRCLE_FINE_GRADE_SECONDARY_STEPS; ++i) {
        float angle_i = i * grade_step;
        float out_radius = (i % 2 == 0) ? 0.5f * CIRCLE_FINE_GRADE_PRIMARY_OUT_RADIUS : 0.5f * CIRCLE_FINE_GRADE_SECONDARY_OUT_RADIUS;
        builder.add_vertex({ { 0.5f * cos(angle_i), 0.5f * sin(angle_i), 0.0f } });
        builder.add_vertex({ { out_radius * cos(angle_i), out_radius * sin(angle_i), 0.0f } });
    }
    vertices_count += 2 * CIRCLE_FINE_GRADE_SECONDARY_STEPS;
    grade_step = float(TWO_PI) / CIRCLE_COARSE_GRADE_STEPS;
    for (uint8_t i = 0; i < CIRCLE_COARSE_GRADE_STEPS; ++i) {
        float angle_i = i * grade_step;
        builder.add_vertex({ { 0.5f * CIRCLE_COARSE_GRADE_IN_RADIUS * cos(angle_i), 0.5f * CIRCLE_COARSE_GRADE_IN_RADIUS * sin(angle_i), 0.0f } });
        builder.add_vertex({ { 0.5f * CIRCLE_COARSE_GRADE_OUT_RADIUS * cos(angle_i), 0.5f * CIRCLE_COARSE_GRADE_OUT_RADIUS * sin(angle_i), 0.0f } });
    }
    vertices_count += 2 * CIRCLE_COARSE_GRADE_STEPS;
    builder
        .add_draw_command({ Render::PrimitiveType::Lines, 0, vertices_count,  Render::Material{} });
    m_geometry_manager.set(GizmoDataId::GradedCircle, builder.build(m_device));
}

} // namespace Slic3r::App::Plater
