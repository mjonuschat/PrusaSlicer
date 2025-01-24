#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Log.hpp"
#include "Slic3r/Assert.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

#include <cfloat>

namespace Slic3r::Domain {

static bool check_texture(const std::string& filename)
{
    boost::system::error_code ec; // so the exists call does not throw (e.g. after a permission problem)
    return !filename.empty() &&
           (boost::algorithm::iends_with(filename, ".png") ||
            boost::algorithm::iends_with(filename, ".svg")) &&
           boost::filesystem::exists(filename, ec);
}

static bool check_model(const std::string& filename)
{
    boost::system::error_code ec;
    return !filename.empty() && boost::algorithm::iends_with(filename, ".stl") &&
        boost::filesystem::exists(filename, ec);
}

Bed Bed::from(
    const Pointfs& contour,
    float max_print_height,
    const std::string& model_filename,
    const std::string& texture_filename
)
{
    Bed ret;
    ret.m_contour = contour;
    ret.m_max_print_height = max_print_height;
    ret.m_model_filename = model_filename;
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

    ret.m_contour_aabb = max - min;
    ret.m_center = 0.5 * (min + max);
    return ret;
}

}
