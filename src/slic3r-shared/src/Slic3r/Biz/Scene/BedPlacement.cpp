#include "Slic3r/Biz/Scene/BedPlacement.hpp"
#include "Slic3r/Biz/Scene/BedGeometry.hpp"
#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Domain/Project.hpp"

#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Domain/Transformation.hpp"

namespace Slic3r::Biz::Scene {

Vec2d max(const Vec2d& v1, const Vec2d& v2)
{
    return { std::max(v1.x(), v2.x()), std::max(v1.y(), v2.y()) };
}

void BedPlacement::layout(Domain::Project& project, const Vec2d& gap)
{
    using Algorithms::BoundingBox::sizes;

    Domain::Project::ConfigContainerList& ccs = project.config_containers();
    double offset_y = 0.0;
    for (size_t i = 0; i < ccs.size(); ++i) {
        auto& cc = ccs[i];
        const Domain::Bed& bed = cc->bed();
        Vec2d size = bed.contour_aabb_extent();
        Vec2d pos = offset_y * Vec2d::UnitY();
        Domain::TriangleMesh model = BedGeometry::model(bed);
        if (!model.empty())
            size = max(size, to_2d(sizes(model.bounding_box())));

        Domain::ConfigContainer::BedInstanceList& instances = cc->bed_instances();
        for (size_t j = 0; j < instances.size(); ++j) {
            if (j > 0)
                pos.x() += size.x() + gap.x();
            Transform3d xform = Domain::translation_transform(to_3d(pos, 0.0));
            instances[j]->transformation = Domain::Transformation(xform);
        }

        offset_y += size.y() + gap.y();
    }
}

} // namespace Slic3r::Biz::Scene

