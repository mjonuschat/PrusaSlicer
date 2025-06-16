#pragma once

#include "Slic3r/Domain/Axis.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/ModelVolume.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Biz::Algorithms::ModelVolume {

Domain::ModelVolume* construct_ptr(Domain::ModelObject* object, const Domain::TriangleMesh& mesh, Domain::ModelVolumeType type = Domain::ModelVolumeType::MODEL_PART);

Domain::ModelVolume* construct_ptr(Domain::ModelObject* object, Domain::TriangleMesh&& mesh, Domain::ModelVolumeType type = Domain::ModelVolumeType::MODEL_PART);

Domain::ModelVolume* construct_ptr(Domain::ModelObject* object, const Domain::ModelVolume& other, Domain::TriangleMesh&& mesh);

void translate(Domain::ModelVolume& model_volume, double x, double y, double z);

void translate(Domain::ModelVolume& model_volume, const Domain::Vec3d& displacement);

void scale(Domain::ModelVolume& model_volume, const Domain::Vec3d& scaling_factors);

void scale(Domain::ModelVolume& model_volume, double x, double y, double z);

void scale(Domain::ModelVolume& model_volume, double s);

void rotate(Domain::ModelVolume& model_volume, double angle, Domain::Axis axis);

void rotate(Domain::ModelVolume& model_volume, double angle, const Domain::Vec3d& axis);

void mirror(Domain::ModelVolume& model_volume, Domain::Axis axis);

bool is_splittable(const Domain::ModelVolume& model_volume);

/**
 * Translates the mesh and the convex hull so that the origin of their vertices is in the center of this volume's bounding box.
 * Attention! This method may only be called just after ModelVolume creation! It must not be called once the TriangleMesh of this ModelVolume is shared!
 */
void center_geometry_after_creation(Domain::ModelVolume& model_volume, bool update_source_offset = true);

void calculate_convex_hull(Domain::ModelVolume& model_volume);

} // namespace Slic3r::Biz::Algorithms::ModelVolume

namespace cereal {

template<class Archive>
void load(Archive& ar, Slic3r::Domain::ModelVolume& model_volume)
{
    bool has_convex_hull;
    ar(cereal::base_class<Slic3r::Domain::ObjectBase>(&model_volume), model_volume.name, model_volume.source, model_volume.m_mesh, model_volume.m_type, model_volume.m_transformation, model_volume.m_is_splittable, has_convex_hull, model_volume.cut_info);
    cereal::load_by_value(ar, model_volume.supported_facets);
    cereal::load_by_value(ar, model_volume.seam_facets);
    cereal::load_by_value(ar, model_volume.mm_segmentation_facets);
    cereal::load_by_value(ar, model_volume.fuzzy_skin_facets);
    cereal::load_by_value(ar, model_volume.volume_settings);
    cereal::load(ar, model_volume.text_configuration);
    cereal::load(ar, model_volume.emboss_shape);
    assert(model_volume.m_mesh);
    if (has_convex_hull) {
        cereal::load_optional(ar, model_volume.m_convex_hull);
        if (!model_volume.m_convex_hull && !model_volume.m_mesh->empty()) {
            // The convex hull was released from the Undo / Redo stack to conserve memory. Recalculate it.
            Slic3r::Biz::Algorithms::ModelVolume::calculate_convex_hull(model_volume);
        }
    } else {
        model_volume.m_convex_hull.reset();
    }
}

template<class Archive>
void save(Archive& ar, const Slic3r::Domain::ModelVolume& model_volume)
{
    bool has_convex_hull = model_volume.m_convex_hull != nullptr;
    ar(cereal::base_class<Slic3r::Domain::ObjectBase>(&model_volume), model_volume.name, model_volume.source, model_volume.m_mesh, model_volume.m_type, model_volume.m_transformation, model_volume.m_is_splittable, has_convex_hull, model_volume.cut_info);
    cereal::save_by_value(ar, model_volume.supported_facets);
    cereal::save_by_value(ar, model_volume.seam_facets);
    cereal::save_by_value(ar, model_volume.mm_segmentation_facets);
    cereal::save_by_value(ar, model_volume.fuzzy_skin_facets);
    cereal::save_by_value(ar, model_volume.volume_settings);
    cereal::save(ar, model_volume.text_configuration);
    cereal::save(ar, model_volume.emboss_shape);
    if (has_convex_hull) {
        cereal::save_optional(ar, model_volume.m_convex_hull);
    }
}

} // namespace cereal
