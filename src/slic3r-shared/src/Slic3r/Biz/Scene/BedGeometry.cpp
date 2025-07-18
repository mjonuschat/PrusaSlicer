#include "Slic3r/Biz/Scene/BedGeometry.hpp"
#include "Slic3r/Biz/Algorithms/ExPolygon.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Biz/Algorithms/Tesselate.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/ExPolygon.hpp"
#include "Slic3r/Domain/Line.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <Slic3r/Log.hpp>
#include <Slic3r/Assert.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <cfloat>

using Slic3r::Domain::BoundingBox2crd;
using Slic3r::Domain::ExPolygon;
using Slic3r::Domain::Line;
using Slic3r::Domain::Lines;
using Slic3r::Domain::Vec2d;
using Slic3r::Domain::Vec2f;
using Slic3r::Domain::Vec3f;

using namespace Slic3r::Biz;

namespace Slic3r::Biz::Scene {

using Domain::TriangleMesh;

TriangleMesh BedGeometry::model(const Domain::Bed& bed)
{
    namespace TriMesh = Biz::Algorithms::TriangleMesh;
    const std::string& model_filename = bed.model_filename();
    std::optional<TriangleMesh> mesh{TriMesh::read_stl_file(model_filename.c_str())};

    const bool res = !model_filename.empty() && mesh;
    if (!res) {
        SPDLOG_ERROR("Unable to load bed model from file: {}", model_filename);
        return TriangleMesh{};
    }

    mesh->translate(Algorithms::Point::to_3d(bed.center(), 0.0).cast<float>());
    return *mesh;
}

std::vector<std::pair<Vec3f, Vec2f>> BedGeometry::plate_triangles(const Domain::Bed& bed)
{
    std::vector<std::pair<Vec3f, Vec2f>> ret;

    ExPolygon contour = ExPolygon(Algorithms::Polygon::scaled(bed.contour()));
    BoundingBox2crd bbox = Algorithms::Polygon::get_extents(contour.contour);
    if (!bbox.defined) {
        SPDLOG_ERROR("Invalid bed contour");
        return ret;
    }

    using Slic3r::Biz::Algorithms::Tesselate::triangulate_expolygon_2f;
    using Slic3r::Biz::Algorithms::Tesselate::NORMALS_UP;
    std::vector<Vec2f> triangles = triangulate_expolygon_2f(contour, NORMALS_UP);
    if (triangles.empty() || triangles.size() % 3 != 0) {
        SPDLOG_ERROR("Unable to triangulate bed contour");
        return ret;
    }

    Vec2f min = { FLT_MAX, FLT_MAX };
    Vec2f max = { -FLT_MAX, -FLT_MAX };
    for (const Vec2f& v : triangles) {
        min.x() = std::min(v.x(), min.x());
        min.y() = std::min(v.y(), min.y());
        max.x() = std::max(v.x(), max.x());
        max.y() = std::max(v.y(), max.y());
    }
    Vec2f size = { max.x() - min.x(), max.y() - min.y() };

    ret.reserve(triangles.size());
    std::transform(triangles.begin(), triangles.end(), std::back_inserter(ret),
        [&min, &size](const Vec2f& v) {
            return std::make_pair(Algorithms::Point::to_3d(v, 0.0f),
                Vec2f((v.x() - min.x()) / size.x(), (v.y() - min.y()) / size.y()));
    });

    return ret;
}

TriangleMesh BedGeometry::plate_mesh(const Domain::Bed& bed)
{
    std::vector<std::pair<Vec3f, Vec2f>> triangles = plate_triangles(bed);
    std::vector<Vec3f> vertices;
    vertices.reserve(triangles.size());
    std::transform(triangles.begin(), triangles.end(), std::back_inserter(vertices),
        [](const std::pair<Vec3f, Vec2f>& v) {
            return v.first;
    });
    std::vector<Domain::Index3> faces;
    faces.reserve(triangles.size() / 3);
    for (int i = 0; i < int(triangles.size()); i += 3) {
        faces.emplace_back(Domain::Index3{i + 0, i + 1, i + 2});
    }

    using Biz::Algorithms::TriangleMesh::construct;
    return construct(vertices, faces);
}

std::vector<Vec3f> BedGeometry::plate_contour(const Domain::Bed& bed)
{
    std::vector<Vec3f> ret;

    ExPolygon contour = ExPolygon(Algorithms::Polygon::scaled(bed.contour()));
    BoundingBox2crd bbox = Algorithms::Polygon::get_extents(contour.contour);
    if (!bbox.defined) {
        SPDLOG_ERROR("Invalid bed contour");
        return ret;
    }

#if 0
    // fake hole for test
    if (contour.holes.empty()) {
        Polygons holes = offset(contour, scale_(-75.0f));
        for (Polygon& p : holes) {
            contour.holes.emplace_back(p);
        }
    }
#endif

    Lines lines = Algorithms::ExPolygon::to_lines(contour);
    ret.reserve(2 * lines.size());
    for (const Line& l : lines) {
        ret.emplace_back(Algorithms::Point::to_3d(Algorithms::Scaling::unscaled(l.a), 0.0).cast<float>());
        ret.emplace_back(Algorithms::Point::to_3d(Algorithms::Scaling::unscaled(l.b), 0.0).cast<float>());
    }
    return ret;
}

std::vector<Vec3f> BedGeometry::print_volume(const Domain::Bed& bed)
{
    std::vector<Vec3f> ret;

    ExPolygon contour = ExPolygon(Algorithms::Polygon::scaled(bed.contour()));
    BoundingBox2crd bbox = Algorithms::Polygon::get_extents(contour.contour);
    if (!bbox.defined) {
        SPDLOG_ERROR("Invalid bed contour");
        return ret;
    }

    float max_print_height = bed.max_print_height();

    Lines lines = Algorithms::ExPolygon::to_lines(contour);
    ret.reserve(6 * lines.size());
    for (const Line& l : lines) {
        ret.emplace_back(Algorithms::Point::to_3d(Algorithms::Scaling::unscaled(l.a), 0.0).cast<float>());
        ret.emplace_back(Algorithms::Point::to_3d(Algorithms::Scaling::unscaled(l.b), 0.0).cast<float>());
        ret.emplace_back(Algorithms::Point::to_3d(Algorithms::Scaling::unscaled(l.a), max_print_height).cast<float>());
        ret.emplace_back(Algorithms::Point::to_3d(Algorithms::Scaling::unscaled(l.b), max_print_height).cast<float>());
        ret.emplace_back(Algorithms::Point::to_3d(Algorithms::Scaling::unscaled(l.a), 0.0).cast<float>());
        ret.emplace_back(Algorithms::Point::to_3d(Algorithms::Scaling::unscaled(l.a), max_print_height).cast<float>());
    }
    return ret;
}

TriangleMesh BedGeometry::axis(const Domain::Bed& bed)
{
    const Vec2d& aabb_extent = bed.contour_aabb_extent();
    double height = double(bed.max_print_height());
    static constexpr double SCALE_FACTOR = 0.1;
    static constexpr double STEM_LENGTH_FACTOR = 0.8;
    static constexpr double CONE_LENGTH_FACTOR = 1.0 - STEM_LENGTH_FACTOR;
    static constexpr double STEM_RADIUS = 0.5;
    static constexpr double CONE_RADIUS = 3.0 * STEM_RADIUS;
    double length = SCALE_FACTOR * std::max({aabb_extent.x(), aabb_extent.y(), height});
    double stem_length = STEM_LENGTH_FACTOR * length;
    double cone_height = CONE_LENGTH_FACTOR * length;

    namespace TriMesh = Biz::Algorithms::TriangleMesh;
    TriangleMesh ret = TriMesh::make_cylinder(STEM_RADIUS, stem_length);
    TriangleMesh cone_tip = TriMesh::make_cone(CONE_RADIUS, cone_height);
    cone_tip.translate({0.0f, 0.0f, float(stem_length)});
    ret.merge(cone_tip);
    return ret;
}

std::vector<std::pair<Domain::Vec3f, Domain::Vec2f>> BedGeometry::label(const Domain::Bed& bed, float width, float height)
{
    std::vector<std::pair<Vec3f, Vec2f>> ret;
    ret = {
        { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { width, 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { width, height, 0.0f }, { 1.0f, 1.0f } },
        { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { width, height, 0.0f }, { 1.0f, 1.0f } },
        { { 0.0f, height, 0.0f }, { 0.0f, 1.0f } }
    };
    return ret;
}

} // namespace Slic3r::Biz::Scene
