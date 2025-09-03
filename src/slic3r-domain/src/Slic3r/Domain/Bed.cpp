#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

#include <cfloat>

namespace Slic3r::Domain {

static bool check_texture(const std::string& filename)
{
    boost::system::error_code ec; // so the exists call does not throw (e.g. after a permission problem)
    return !filename.empty()
        && (boost::algorithm::iends_with(filename, ".png") || boost::algorithm::iends_with(filename, ".svg"))
        && boost::filesystem::exists(filename, ec);
}

static bool check_model(const std::string& filename)
{
    boost::system::error_code ec;
    return !filename.empty() && boost::algorithm::iends_with(filename, ".stl") && boost::filesystem::exists(filename, ec);
}

Bed Bed::from(const Vec2ds& contour, float max_print_height, const std::optional<Bed::Segments>& bed_segments, const std::string& model_filename, const std::string& texture_filename)
{
    Bed ret;
    ret.m_contour          = contour;
    ret.m_max_print_height = max_print_height;
    ret.m_segments         = bed_segments;
    ret.m_model_filename   = model_filename;
    ret.m_texture_filename = texture_filename;

    if (!ret.m_model_filename.empty() && !check_model(ret.m_model_filename)) {
        SPDLOG_WARN("Invalid or unreachable bed model: {}", ret.m_model_filename);
        ret.m_model_filename.clear();
    }

    if (!ret.m_texture_filename.empty() && !check_texture(ret.m_texture_filename)) {
        SPDLOG_WARN("Invalid or unreachable bed texture: {}", ret.m_texture_filename);
        ret.m_texture_filename.clear();
    }

    Vec2d min = {DBL_MAX, DBL_MAX};
    Vec2d max = {-DBL_MAX, -DBL_MAX};

    for (const Vec2d& v : ret.m_contour) {
        min.x() = std::min(v.x(), min.x());
        min.y() = std::min(v.y(), min.y());
        max.x() = std::max(v.x(), max.x());
        max.y() = std::max(v.y(), max.y());
    }

    ret.m_contour_aabb_extent = max - min;
    ret.m_center              = 0.5 * (min + max);
    // TODO: calculate offset as done in libslic3r BuildVolume
    // ret.m_offset = /*TODO*/;
    return ret;
}

static bool vec2d_equal(const Vec2d& a, const Vec2d& b)
{
    return Domain::fuzzy_compare(a.x(), b.x()) && Domain::fuzzy_compare(a.y(), b.y());
}

bool Bed::operator==(const Bed& rhs) const
{
    if (m_type != rhs.m_type) {
        return false;
    }
    if (!vec2d_equal(m_center, rhs.m_center)) {
        return false;
    }
    if (!vec2d_equal(m_offset, rhs.m_offset)) {
        return false;
    }
    if (!vec2d_equal(m_contour_aabb_extent, rhs.m_contour_aabb_extent)) {
        return false;
    }
    if (!Domain::fuzzy_compare(m_max_print_height, rhs.m_max_print_height)) {
        return false;
    }
    if (m_model_filename != rhs.m_model_filename) {
        return false;
    }
    if (m_texture_filename != rhs.m_texture_filename) {
        return false;
    }
    if (m_segments != rhs.m_segments) {
        return false;
    }
    if (m_contour.size() != rhs.m_contour.size()) {
        return false;
    }
    for (size_t i = 0; i < m_contour.size(); ++i) {
        if (!vec2d_equal(m_contour[i], rhs.m_contour[i])) {
            return false;
        }
    }
    if (m_top_bottom_convex_hull_decomposition != rhs.m_top_bottom_convex_hull_decomposition) {
        return false;
    }
    if (m_circle != rhs.m_circle) {
        return false;
    }
    return true;
}

bool operator==(const Bed::Segments& a, const Bed::Segments& b)
{
    return a.x_count == b.x_count && a.y_count == b.y_count;
}
} // namespace Slic3r::Domain
