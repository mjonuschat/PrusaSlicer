#pragma once

#include "Slic3r/Domain/Bed.hpp"
#include "Slic3r/Domain/BedInstance.hpp"
#include "Slic3r/Biz/Algorithms/Geometry/Circle.hpp"

namespace Slic3r::Biz::Algorithms::Bed {

enum class BedContainmentState
{
    // Inside the build volume, thus printable.
    Inside,
    // Colliding with the build volume boundary, thus not printable and error is shown.
    Colliding,
    // Outside of the build volume means the object is ignored: Not printed and no error is shown.
    Outside,
    // Completely below the print bed. The same as Outside, but an object with one printable part below the print bed
    // and at least one part above the print bed is still printable.
    Below,
};

Domain::BedType detect_bed_type(const Domain::Bed& bed);
// Returns the best fitting circle for the given circular bed.
Geometry::Circled as_circular_bed(const Domain::Bed& bed);

BedContainmentState contains_2d(const Domain::BedInstance& bed_instance, const Domain::BoundingBox2d& object_bb);
BedContainmentState contains_3d(const Domain::BedInstance& bed_instance, const Domain::BoundingBox3d& object_bb);

} // namespace Slic3r::Biz::Algorithms::Bed
