///|/ Copyright (c) Prusa Research 2026 Filip Sykala @Jony01
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Scene/EmbossCreate.hpp"
#include "Slic3r/App/Scene/SceneNodeTag.hpp"
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"

namespace Slic3r::App::Scene {
namespace {
std::optional<Domain::Vec2d> get_z_zero_coor(const Ray& pick_ray) {
    double d_z = pick_ray.direction.z();
    if (fabs(d_z) - 1e-4 <= 0.)
        return std::nullopt; // almost parallel to Z axis solve as no bed under mouse

    // prerequisity: bed is alligned -> parallel with Z plane AND Z = 0
    Domain::Vec3d z0 = pick_ray.point_at(-pick_ray.origin.z() / d_z);
    return Domain::Vec2d(z0.x(), z0.y());
}
} // namespace

bool start_create(Biz::Emboss::CreateVolumeParams& input, const Ray& pick_ray, const NodePickResults& picks)
{
    const Biz::ProjectInteractor& project_interactor = input.base.project_interactor;
    const Domain::ModelInstance* selected_instance = Biz::Emboss::get_selected_instance(project_interactor);
    const Domain::Project& project = project_interactor.selected_project();
    for (const NodePickResult& pick : picks) {
        if (pick.node->has_tag_of_type<SceneNodeTag>()) {
            const auto* tag = pick.node->tag_of_type<SceneNodeTag>();
            const Domain::ModelVolume* volume = project.find_volume_by_id(tag->object_id, tag->volume_id);
            if (volume == nullptr)
                continue; // no volume under mouse
            // TODO: What to do with Negative volume
            if (volume->type() != Domain::ModelVolumeType::MODEL_PART)
                continue; // skip modifiers + SupportBlock/Enforce

            Domain::Vec3d pick_point = pick_ray.point_at(pick.cast.distance);
            Domain::Vec3d pick_normal = pick.cast.normal;
            // normal could be from scaled object it needs normalize
            pick_normal.normalize();

            const Domain::ModelObject& object = selected_instance != nullptr ?
                *selected_instance->get_object() : *volume->get_object();
            const Domain::ModelInstance* instance = project.find_instance_by_id(object.id().id, tag->instance_id);
            Domain::Transform3d surface_trmat = Biz::Emboss::create_transformation_onto_surface(pick_point, pick_normal, Biz::Emboss::UP_LIMIT);
            Domain::Transform3d tr = instance->get_matrix().inverse() * surface_trmat;
            return Biz::Emboss::start_create_volume_job(*instance, tr, input.base, input.volume_type);
        }
    }

    auto bed_coor = get_z_zero_coor(pick_ray);
    if (bed_coor.has_value())
    {
        if (selected_instance == nullptr) {
            return Biz::Emboss::start_create_object_job(input, *bed_coor);
        }
        else {
            Domain::Vec3d pick_point(bed_coor->x(), bed_coor->y(), 0.);
            Domain::Vec3d pick_normal(0., 0., 1.);
            Domain::Transform3d surface_trmat = Biz::Emboss::create_transformation_onto_surface(pick_point, pick_normal, Biz::Emboss::UP_LIMIT);
            Domain::Transform3d tr = selected_instance->get_matrix().inverse() * surface_trmat;
            return Biz::Emboss::start_create_volume_job(*selected_instance, tr, input.base, input.volume_type);
        }
    }
    return Biz::Emboss::start_create_object_job(input, Domain::Vec2d(0, 0)); // fall back, do not use pick ray
}

bool start_create_volume(Biz::Emboss::CreateVolumeParams& input, const Ray& pick_ray, const NodePickResults& picks) {
    const Biz::ProjectInteractor& project_interactor = input.base.project_interactor;
    const Domain::ModelInstance* selected_instance = Biz::Emboss::get_selected_instance(project_interactor);
    ASSERT(selected_instance != nullptr); // no object selected
    const Domain::Project& project = project_interactor.selected_project();
    for (const NodePickResult& pick : picks) {
        if (pick.node->has_tag_of_type<SceneNodeTag>()) {
            const auto* tag = pick.node->tag_of_type<SceneNodeTag>();
            Domain::SelectionId instance_id = selected_instance->id().id;
            Domain::SelectionId object_id = selected_instance->get_object()->id().id;
            if (tag->instance_id != instance_id ||
                tag->object_id != object_id)
                continue; // not selected instance

            const Domain::ModelVolume* volume = project.find_volume_by_id(tag->object_id, tag->volume_id);
            if (volume == nullptr)
                continue; // no volume under mouse

            // TODO: What to do with Negative volume
            if (volume->type() != Domain::ModelVolumeType::MODEL_PART)
                continue; // skip modifiers + SupportBlock/Enforce

            Domain::Vec3d pick_point = pick_ray.point_at(pick.cast.distance);
            Domain::Vec3d pick_normal = pick.cast.normal;
            // normal could be from scaled object it needs normalize
            pick_normal.normalize();

            Domain::Transform3d surface_trmat = Biz::Emboss::create_transformation_onto_surface(pick_point, pick_normal, Biz::Emboss::UP_LIMIT);
            Domain::Transform3d tr = selected_instance->get_matrix().inverse() * surface_trmat;
            return Biz::Emboss::start_create_volume_job(*selected_instance, tr, input.base, input.volume_type);
        }
    }
    // create volume near selected instance
    const Domain::ModelObject* object = selected_instance->get_object();
    Domain::BoundingBox3d bb;
    for (const auto v : object->volumes) {
        bb = Biz::Algorithms::BoundingBox::merge(bb,
            Biz::Algorithms::BoundingBox::transformed(
                v->get_convex_hull().bounding_box(), v->get_matrix())
        );
    }
    bb = Biz::Algorithms::BoundingBox::transformed(bb, selected_instance->get_matrix());
    Domain::Vec3d point((bb.min.x() + bb.max.x()) / 2., bb.min.y(), 0); // x -> object middle, y -> object min
    Domain::Transform3d surface_trmat = Biz::Emboss::create_transformation_onto_surface(point, Domain::Vec3d::UnitY(), Biz::Emboss::UP_LIMIT);
    Domain::Transform3d tr = selected_instance->get_matrix().inverse() * surface_trmat;
    return Biz::Emboss::start_create_volume_job(*selected_instance, tr, input.base, input.volume_type);
}

// ignore selection and create object in center ray direction
bool start_create_object(Biz::Emboss::CreateVolumeParams& input, const Ray& pick_ray, const NodePickResults& picks) {
    auto bed_coor = get_z_zero_coor(pick_ray);
    if (!bed_coor.has_value())
        return Biz::Emboss::start_create_object_job(input, Domain::Vec2d(0, 0)); // fall back, do not use pick ray    
    return Biz::Emboss::start_create_object_job(input, *bed_coor);
}

} // namespace Slic3r::App::Scene
