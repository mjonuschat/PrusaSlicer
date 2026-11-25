#include "boss/features/exclude-object-skirt-brim-wipe/ExcludeObjectSkirtBrimWipeFeature.hpp"

#include <algorithm>
#include <cstdio>
#include <string>

#include "Slic3r/Biz/Algorithms/DouglasPeucker.hpp"
#include "libslic3r/Geometry/ConvexHull.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/Print.hpp"

namespace Slic3r::Boss {

namespace {

std::string sanitize_klipper_name(std::string name)
{
    // Klipper's EXCLUDE_OBJECT_DEFINE parser and common filesystem/shell tooling both choke on
    // this set, so it is banned the same way LabelObjects.cpp bans it for per-instance names.
    const std::string banned = "\b\t\n\v\f\r \"#%&\'*-./:;<>\\";
    std::replace_if(name.begin(), name.end(), [&banned](char c) { return banned.find(c) != std::string::npos; }, '_');
    return name;
}

std::pair<std::string, std::string> format_center_and_polygon(Polygon outline)
{
    Biz::Algorithms::DouglasPeucker::douglas_peucker(outline, 50000.f);
    Point center = outline.centroid();
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer) - 1, "%.3f,%.3f", unscale<float>(center[0]), unscale<float>(center[1]));
    std::string center_str(buffer);
    std::string polygon_str = std::string("[");
    for (const Point &point : outline) {
        std::snprintf(buffer, sizeof(buffer) - 1, "[%.3f,%.3f],", unscale<float>(point[0]), unscale<float>(point[1]));
        polygon_str += buffer;
    }
    polygon_str.pop_back();
    polygon_str += "]";
    return {center_str, polygon_str};
}

} // namespace

std::vector<LabelObjectsDeclaration> ExcludeObjectSkirtBrimWipeFeature::declare(const LabelObjectsPolicyContext &ctx)
{
    std::vector<LabelObjectsDeclaration> result;

    if (! ctx.print->skirt_convex_hull().empty()) {
        auto [center, polygon] = format_center_and_polygon(Geometry::convex_hull(ctx.print->skirt_convex_hull()));
        result.push_back({sanitize_klipper_name("Skirt/Brim"), center, polygon});
    }

    // wipe_tower_data() reports whether this print currently has a wipe tower, unlike
    // can_have_wipe_tower() which only reports whether the print's configuration could ever
    // produce one -- using the latter would declare a synthetic Wipe Tower object on prints
    // that have no wipe tower at all.
    if (ctx.print->wipe_tower_data().has_value()) {
        Points wipe_tower_pts = ctx.print->first_layer_wipe_tower_corners();
        if (! wipe_tower_pts.empty()) {
            auto [center, polygon] = format_center_and_polygon(Geometry::convex_hull(wipe_tower_pts));
            result.push_back({sanitize_klipper_name("Wipe Tower"), center, polygon});
        }
    }

    return result;
}

} // namespace Slic3r::Boss
