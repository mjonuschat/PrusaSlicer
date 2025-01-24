#include "Slic3r/Biz/Scene/BedPlacement.hpp"
#include "Slic3r/Biz/Plater/BedGeometry.hpp"
#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Domain/Project.hpp"

#include <libslic3r/TriangleMesh.hpp>

namespace Slic3r::Biz::Scene {

static Vec2d max(const Vec2d& v1, const Vec2d& v2)
{
    return { std::max(v1.x(), v2.x()), std::max(v1.y(), v2.y()) };
}

void BedPlacement::layout(Domain::Project& project, const Vec2d& gap)
{
    Domain::Project::ConfigContainerList& ccs = project.config_containers();
    double offset_y = 0.0;
    for (size_t i = 0; i < ccs.size(); ++i) {
        auto& cc = ccs[i];
        const Domain::Bed& bed = cc->bed();
        Vec2d size = bed.contour_aabb();
        Vec2d pos = offset_y * Vec2d::UnitY();
        TriangleMesh model = Biz::Plater::BedGeometry::model(bed);
        if (!model.empty())
            size = max(size, to_2d(model.bounding_box().size()));

        Domain::ConfigContainer::BedInstanceList& instances = cc->bed_instances();
        for (size_t j = 0; j < instances.size(); ++j) {
            if (j > 0)
                pos.x() += size.x() + gap.x();
            Transform3d xform = Geometry::translation_transform(to_3d(pos, 0.0));
            instances[j]->set_transformation(Geometry::Transformation(xform));
        }

        offset_y += size.y() + gap.y();
    }
}

} // namespace Slic3r::Biz::Scene

