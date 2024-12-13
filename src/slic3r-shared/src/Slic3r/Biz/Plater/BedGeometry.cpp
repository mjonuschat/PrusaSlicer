#include "Slic3r/Biz/Plater/BedGeometry.hpp"
#include "Slic3r/Domain/Bed.hpp"

#include <libslic3r/Point.hpp>
#include <libslic3r/Tesselate.hpp>
#include <libslic3r/ClipperUtils.hpp>

#include <Slic3r/Log.hpp>
#include <Slic3r/Assert.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <cfloat>

/**
  * @brief Z offset to prevent z-fighting
  */
static constexpr double GROUND_Z = -0.005;

namespace Slic3r::Biz::Plater {

TriangleMesh BedGeometry::model(const Domain::Bed& bed)
{
    std::string model_filename = bed.model_filename();
    DEBUG_ASSERT(!model_filename.empty());
    TriangleMesh ret;
    bool res = !model_filename.empty() && ret.ReadSTLFile(model_filename.c_str());
    if (res)
        ret.translate(to_3d(bed.center(), 3.0 * GROUND_Z).cast<float>());
    else
        SPDLOG_ERROR("Unable to load bed model from file: {}", model_filename);
    return ret;
}

std::vector<std::pair<Vec3f, Vec2f>> BedGeometry::plate_triangles(const Domain::Bed& bed)
{
    std::vector<std::pair<Vec3f, Vec2f>> ret;

    ExPolygon contour = ExPolygon(Polygon::new_scale(bed.contour()));
    BoundingBox bbox = contour.contour.bounding_box();
    if (!bbox.defined) {
        SPDLOG_ERROR("Invalid bed contour");
        return ret;
    }

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
            return std::make_pair(to_3d(v, 2.0f * float(GROUND_Z)),
                Vec2f((v.x() - min.x()) / size.x(), (v.y() - min.y()) / size.y()));
    });

    return ret;
}

std::vector<Vec3f> BedGeometry::plate_contour(const Domain::Bed& bed)
{
    std::vector<Vec3f> ret;

    ExPolygon contour = ExPolygon(Polygon::new_scale(bed.contour()));
    BoundingBox bbox = contour.contour.bounding_box();
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

    Lines lines = to_lines(contour);
    ret.reserve(2 * lines.size());
    for (const Slic3r::Line& l : lines) {
        ret.emplace_back(to_3d(unscale(l.a), GROUND_Z).cast<float>());
        ret.emplace_back(to_3d(unscale(l.b), GROUND_Z).cast<float>());
    }
    return ret;
}

std::vector<Vec3f> BedGeometry::print_volume(const Domain::Bed& bed)
{
    std::vector<Vec3f> ret;

    ExPolygon contour = ExPolygon(Polygon::new_scale(bed.contour()));
    BoundingBox bbox = contour.contour.bounding_box();
    if (!bbox.defined) {
        SPDLOG_ERROR("Invalid bed contour");
        return ret;
    }

    float max_print_height = bed.max_print_height();

    Lines lines = to_lines(contour);
    ret.reserve(6 * lines.size());
    for (const Slic3r::Line& l : lines) {
        ret.emplace_back(to_3d(unscale(l.a), GROUND_Z).cast<float>());
        ret.emplace_back(to_3d(unscale(l.b), GROUND_Z).cast<float>());
        ret.emplace_back(to_3d(unscale(l.a), max_print_height).cast<float>());
        ret.emplace_back(to_3d(unscale(l.b), max_print_height).cast<float>());
        ret.emplace_back(to_3d(unscale(l.a), GROUND_Z).cast<float>());
        ret.emplace_back(to_3d(unscale(l.a), max_print_height).cast<float>());
    }
    return ret;
}

} // namespace Slic3r::Biz::Plater
