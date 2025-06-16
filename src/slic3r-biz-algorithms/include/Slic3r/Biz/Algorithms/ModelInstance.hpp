#pragma once

#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/ModelInstance.hpp"
#include "Slic3r/Domain/TriangleMesh.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <cereal/types/base_class.hpp>

namespace Slic3r::Biz::Algorithms::ModelInstance {

/**
 * Transform an external bounding box, thus the resulting bounding box is no more snug.
 */
Domain::BoundingBox3d transformed_bounding_box(const Domain::BoundingBox3d& bbox, const Domain::ModelInstance& model_instance, bool dont_translate = false);

/**
 * To be called on an external mesh.
 */
void transform_mesh(Domain::TriangleMesh& mesh, const Domain::ModelInstance& model_instance, bool dont_translate = false);

/**
 * Transform an external vector.
 */
Domain::Vec3d transformed_vector(const Domain::Vec3d& v, const Domain::ModelInstance& model_instance, bool dont_translate = false);

/**
 * To be called on an external polygon. It does not translate the polygon, only rotates and scales.
 */
void transform_polygon(Domain::Polygon& polygon, const Domain::ModelInstance& model_instance);

} // namespace Slic3r::Biz::Algorithms::ModelInstance

namespace cereal {

template<class Archive>
void serialize(Archive& ar, Slic3r::Domain::ModelInstance& model_instance)
{
    ar(cereal::base_class<Slic3r::Domain::ObjectBase>(&model_instance), model_instance.m_transformation, model_instance.print_volume_state, model_instance.printable);
}

} // namespace cereal
