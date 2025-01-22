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

} // namespace


void GizmoDataFactory::create_data(GizmoDataId id)
{
    indexed_triangle_set its;

    switch (id) {

    case GizmoDataId::Segment: 
        // creates a segment 1 unit long, along X axis
        create_segment();
        return;

    case GizmoDataId::Cone:
        // creates a cone contained into a 1x1x1 box 
        // the cone axis is the Z axis
        its = its_make_cone(0.5, 1.0, ANGLE_STEP);
        break;

    case GizmoDataId::Cube:
        // creates an axis aligned 1x1x1 cube 
        its = its_make_cube(1.0, 1.0, 1.0);
        its_translate(its, -0.5f * Vec3f::Ones());
        break;

    case GizmoDataId::Circle: 
        // creates a circle with diameter equal to 1 
        // the circle is contained into XY plane
        create_circle();
        return;

    case GizmoDataId::GradedCircle: 
        // creates a graded circle with diameter equal to 1 
        // the circle is contained into XY plane
        create_graded_circle();
        return;

    default:
        UNREACHABLE("Invalid GizmoDataId", id);
    }

    m_geometry_manager.set(id, Render::geometry_from_triangle_mesh(m_device, its, Render::Material()));
    m_triangle_mesh_manager.set(id, std::make_unique<Scene::TriangleMesh>(std::move(its)));
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
