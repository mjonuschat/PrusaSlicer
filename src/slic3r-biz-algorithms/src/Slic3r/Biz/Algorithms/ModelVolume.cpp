#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Domain/TriangleMesh.hpp"

using namespace Slic3r::Biz::Algorithms;

namespace Slic3r::Biz::Algorithms::ModelVolume {

Domain::ModelVolume *construct_ptr(Domain::ModelObject* object, const Domain::TriangleMesh& mesh, Domain::ModelVolumeType type)
{
    Domain::ModelVolume* volume = new Domain::ModelVolume(object, mesh, type);
    if (volume->m_mesh->facets_count() > 1) {
        ModelVolume::calculate_convex_hull(*volume);
    }

    return volume;
}

Domain::ModelVolume *construct_ptr(Domain::ModelObject* object, Domain::TriangleMesh&& mesh, Domain::ModelVolumeType type)
{
    Domain::ModelVolume* volume = new Domain::ModelVolume(object, std::move(mesh), type);
    if (volume->m_mesh->facets_count() > 1) {
        ModelVolume::calculate_convex_hull(*volume);
    }

    return volume;
}

Domain::ModelVolume *construct_ptr(Domain::ModelObject* object, const Domain::ModelVolume& other, Domain::TriangleMesh&& mesh)
{
    Domain::ModelVolume* volume = new Domain::ModelVolume(object, other, std::move(mesh));
    if (volume->m_mesh->facets_count() > 1) {
        ModelVolume::calculate_convex_hull(*volume);
    }

    return volume;
}

void translate(Domain::ModelVolume& model_volume, const double x, const double y, const double z)
{
    ModelVolume::translate(model_volume, Domain::Vec3d(x, y, z));
}

void translate(Domain::ModelVolume& model_volume, const Domain::Vec3d& displacement)
{
    model_volume.set_offset(model_volume.get_offset() + displacement);
}

void scale(Domain::ModelVolume& model_volume, const Domain::Vec3d& scaling_factors)
{
    model_volume.set_scaling_factor(model_volume.get_scaling_factor().cwiseProduct(scaling_factors));
}

void scale(Domain::ModelVolume& model_volume, const double x, const double y, const double z)
{
    ModelVolume::scale(model_volume, Domain::Vec3d(x, y, z));
}

void scale(Domain::ModelVolume& model_volume, const double s)
{
    ModelVolume::scale(model_volume, Domain::Vec3d(s, s, s));
}

void rotate(Domain::ModelVolume& model_volume, const double angle, const Domain::Axis axis)
{
    switch (axis) {
    case Domain::X: {
        rotate(model_volume, angle, Domain::Vec3d::UnitX());
        break;
    }
    case Domain::Y: {
        rotate(model_volume, angle, Domain::Vec3d::UnitY());
        break;
    }
    case Domain::Z: {
        rotate(model_volume, angle, Domain::Vec3d::UnitZ());
        break;
    }
    default:
        break;
    }
}

void rotate(Domain::ModelVolume& model_volume, const double angle, const Domain::Vec3d& axis)
{
    model_volume.set_rotation(model_volume.get_rotation() + Domain::extract_rotation(Eigen::Quaterniond(Eigen::AngleAxisd(angle, axis)).toRotationMatrix()));
}

void mirror(Domain::ModelVolume& model_volume, const Domain::Axis axis)
{
    Domain::Vec3d mirror = model_volume.get_mirror();
    switch (axis) {
    case Domain::X: {
        mirror.x() *= -1.;
        break;
    }
    case Domain::Y: {
        mirror.y() *= -1.;
        break;
    }
    case Domain::Z: {
        mirror.z() *= -1.;
        break;
    }
    default:
        break;
    }
    model_volume.set_mirror(mirror);
}

bool is_splittable(const Domain::ModelVolume& model_volume)
{
    // The call mesh.is_splittable() is expensive, so cache the value to calculate it only once.
    if (model_volume.m_is_splittable == -1) {
        model_volume.m_is_splittable = TriangleMesh::its_is_splittable(model_volume.mesh().its);
    }

    return model_volume.m_is_splittable == 1;
}

void center_geometry_after_creation(Domain::ModelVolume& model_volume, const bool update_source_offset)
{
    Domain::Vec3d shift = BoundingBox::center(model_volume.mesh().bounding_box());
    if (!shift.isApprox(Domain::Vec3d::Zero()))
    {
        if (model_volume.m_mesh)
            const_cast<Domain::TriangleMesh*>(model_volume.m_mesh.get())->translate(Domain::Vec3f{-(float)shift.x(), -(float)shift.y(), -(float)shift.z()});
        if (model_volume.m_convex_hull)
            const_cast<Domain::TriangleMesh*>(model_volume.m_convex_hull.get())->translate(Domain::Vec3f{-(float)shift.x(), -(float)shift.y(), -(float)shift.z()});
        ModelVolume::translate(model_volume, shift);
    }

    if (update_source_offset) {
        model_volume.source.mesh_offset = shift;
    }
}

void calculate_convex_hull(Domain::ModelVolume& model_volume)
{
    model_volume.m_convex_hull = std::make_shared<Domain::TriangleMesh>(TriangleMesh::convex_hull_3d(model_volume.mesh()));
    assert(model_volume.m_convex_hull.get());
}

Domain::BoundingBox3d transformed_bounding_box(const Domain::ModelVolume& model_volume, const Domain::Transform3d& trafo)
{
    const auto ch = model_volume.get_convex_hull_shared_ptr();
    const Domain::TriangleMesh* m = (ch != nullptr) ? ch.get() : &model_volume.mesh();
    return TriangleMesh::transformed_bounding_box(*m, trafo);
}

} // namespace Slic3r::Biz::Algorithms::ModelVolume
