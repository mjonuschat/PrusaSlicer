#include "Slic3r/App/Plater/BedRenderHelper.hpp"
#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"

#include <libslic3r/ClipperUtils.hpp>

#include <Slic3r/Log.hpp>

#include <boost/algorithm/string/predicate.hpp>

/**
  * @brief Z offset to prevent z-fighting
  */
static constexpr double GROUND_Z = -0.005;

namespace Slic3r::App::Plater {

Render::Texture* BedRenderHelper::texture(const Domain::Bed& bed, size_t size, Render::TextureManager& manager)
{
    std::string texture_filename = bed.texture_filename();

    std::vector<uint8_t> ret;
    if (texture_filename.empty())
        return nullptr;

    Render::ImageLoadOptions opts;
    opts.max_size_px = size;

    return manager.get(texture_filename, opts);
}

std::vector<Vec3f> BedRenderHelper::plate_grid(const Domain::Bed& bed)
{
    std::vector<Vec3f> ret;

    ExPolygon contour = ExPolygon(Polygon::new_scale(bed.contour()));
    BoundingBox bbox = contour.contour.bounding_box();
    if (!bbox.defined) {
        SPDLOG_ERROR("Invalid bed contour");
        return ret;
    }

    static constexpr int32_t STEP = scale_(10.0);
    Polylines gridlines;
    for (int32_t x = bbox.min.x(); x <= bbox.max.x(); x += STEP) {
        gridlines.push_back(Polyline({ x, bbox.min.y() }, { x, bbox.max.y() }));
    }
    for (int32_t y = bbox.min.y(); y <= bbox.max.y(); y += STEP) {
        gridlines.push_back(Polyline({ bbox.min.x(), y }, { bbox.max.x(), y }));
    }
    
    // clip with a slightly grown expolygon because our lines lay on the contours and may get erroneously clipped
    Lines lines = to_lines(intersection_pl(gridlines, offset(contour, float(SCALED_EPSILON))));
    // append bed contours
    Lines contour_lines = to_lines(contour);
    std::copy(contour_lines.begin(), contour_lines.end(), std::back_inserter(lines));

    ret.reserve(2 * lines.size());
    for (const Slic3r::Line& l : lines) {
        ret.emplace_back(to_3d(unscale(l.a), GROUND_Z).cast<float>());
        ret.emplace_back(to_3d(unscale(l.b), GROUND_Z).cast<float>());
    }
    return ret;
}

} // namespace Slic3r::App::Plater
