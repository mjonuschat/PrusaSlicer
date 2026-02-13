#include "Slic3r/Biz/Scene/BedPlacement.hpp"
#include "Slic3r/Biz/Scene/BedGeometry.hpp"
#include "Slic3r/Domain/BedContainer.hpp"
#include "Slic3r/Domain/Project.hpp"
#include "Slic3r/Domain/Types.hpp"

#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/Domain/Transformation.hpp"

using Slic3r::Domain::Transform3d;
using Slic3r::Domain::Vec2d;

namespace Slic3r::Biz::Scene {

Vec2d min(const Vec2d& v1, const Vec2d& v2)
{
    return { std::min(v1.x(), v2.x()), std::min(v1.y(), v2.y()) };
}

Vec2d max(const Vec2d& v1, const Vec2d& v2)
{
    return { std::max(v1.x(), v2.x()), std::max(v1.y(), v2.y()) };
}

Domain::ElementRefs BedPlacement::layout(Domain::Project& project, const Vec2d& gap)
{
    using Algorithms::BoundingBox::sizes;
    Domain::ElementRefs ret;
    Domain::Project::ConfigContainerList& ccs = project.config_containers();
    double offset_y = 0.0;
    for (size_t i = 0; i < ccs.size(); ++i) {
        auto& cc = ccs[i];
        const Domain::Bed& bed = cc->bed();
        const BoundingBoxf& bed_contour_aabb = bed.contour_aabb();
        Vec2d bed_pos = bed_contour_aabb.min;
        Vec2d bed_size = bed.contour_aabb_extent();
        Domain::TriangleMesh model = BedGeometry::model(bed);
        if (!model.empty()) {
            Domain::BoundingBox3d model_aabb = model.bounding_box();
            bed_pos = min(bed_pos, Algorithms::Point::to_2d(model_aabb.min));
            bed_size = max(bed_size, Algorithms::Point::to_2d(sizes(model_aabb)));
        }

        Vec2d pos = offset_y * Vec2d::UnitY() - bed_pos;

        Domain::ConfigContainer::BedInstanceList& instances = cc->bed_instances();
        for (size_t j = 0; j < instances.size(); ++j) {
            if (j > 0)
                pos.x() += bed_size.x() + gap.x();

            Transform3d xform = Domain::translation_transform(Algorithms::Point::to_3d(pos, 0.0));
            Transform3d old_bed_trafo = instances[j]->matrix();
            instances[j]->transformation = Domain::Transformation(xform);
            for (Domain::ModelInstance* mi : instances[j]->model_instances) {
                mi->set_transformation(
                    Domain::Transformation(
                        mi->get_transformation().get_matrix() * xform * old_bed_trafo.inverse()
                    )
                );
                ret.emplace_back(mi->get_object()->id().id, mi->id().id);
            }
        }

        offset_y += bed_size.y() + gap.y();
    }
    return ret;
}

} // namespace Slic3r::Biz::Scene

