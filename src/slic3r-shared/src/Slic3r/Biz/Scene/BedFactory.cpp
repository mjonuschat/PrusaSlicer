#include "Slic3r/Biz/Scene/BedFactory.hpp"

#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Biz/Algorithms/Bed.hpp"
#include "Slic3r/Biz/Algorithms/Geometry/ConvexHull.hpp"

namespace Slic3r::Biz::Scene {

Domain::Bed&
get_or_create_bed(Domain::BedContainer& bed_container, const Domain::Preset::SelectedPreset& preset, const std::string& assets_path)
{
    size_t old_bed_count = bed_container.beds_count();
    Domain::Bed& bed = bed_container.get_or_create_bed(preset, assets_path);

    if (bed_container.beds_count() == old_bed_count) {
        // The bed already existed, no need to detect its type again.
        return bed;
    }

    bed.set_type(Algorithms::Bed::detect_bed_type(bed));
    if (bed.type() == Domain::BedType::Circle) {
        Algorithms::Geometry::Circled circle = Algorithms::Bed::as_circular_bed(bed);
        bed.set_circle({circle.center, circle.radius});
    } else if (bed.type() == Domain::BedType::Convex)
        bed.set_top_bottom_convex_hull_decomposition(Algorithms::Geometry::decompose_convex_polygon_top_bottom(bed.contour()));
    return bed;
}

} // namespace Slic3r::Biz::Scene
