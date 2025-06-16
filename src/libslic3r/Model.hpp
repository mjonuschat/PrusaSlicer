///|/ Copyright (c) Prusa Research 2016 - 2023 Tomáš Mészáros @tamasmeszaros, Oleksandra Iushchenko @YuSanka, Enrico Turri @enricoturri1966, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, Filip Sykala @Jony01, Lukáš Hejl @hejllukas, David Kocík @kocikdav, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2019 John Drake @foxox
///|/ Copyright (c) 2019 Sijmen Schoon
///|/ Copyright (c) 2017 Eyal Soha @eyal0
///|/ Copyright (c) Slic3r 2014 - 2015 Alessandro Ranellucci @alranel
///|/
///|/ ported from lib/Slic3r/Model.pm:
///|/ Copyright (c) Prusa Research 2016 - 2022 Vojtěch Bubník @bubnikv, Enrico Turri @enricoturri1966
///|/ Copyright (c) Slic3r 2012 - 2016 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_Model_hpp_
#define slic3r_Model_hpp_

#include "Slic3r/Domain/FacetsAnnotation.hpp"
#include "Slic3r/Domain/SLA/DrainHole.hpp"
#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "Slic3r/Biz/Algorithms/TriangleMesh.hpp"
#include "Slic3r/Domain/SLA/SupportPoint.hpp"
#include "Slic3r/Biz/Algorithms/Geometry/Geometry.hpp"
#include "Slic3r/Domain/CustomGCode.hpp"
#include "Slic3r/Domain/TextConfiguration.hpp"
#include "Slic3r/Domain/EmbossShape.hpp"
#include "Slic3r/Biz/Algorithms/TriangleSelector.hpp"
#include "Slic3r/Domain/TriangleSelector.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"
#include "Slic3r/Domain/FullConfigSLA.hpp"
#include "Slic3r/Domain/CutConnector.hpp"
#include "Slic3r/Domain/LayerHeightProfile.hpp"
#include "Slic3r/Domain/ModelVolume.hpp"
#include "Slic3r/Domain/ModelObject.hpp"
#include "Slic3r/Domain/ModelInstance.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <random>

namespace Slic3r {

class BuildVolume;
class Print;

using Model = Domain::Model;
using ModelVolume = Domain::ModelVolume;
using ModelObject = Domain::ModelObject;
using ModelInstance = Domain::ModelInstance;
using ModelVolumePtrs = Domain::ModelVolumePtrs;
using ModelObjectPtrs = Domain::ModelObjectPtrs;
using ModelVolumeType = Domain::ModelVolumeType;
using ModelInstancePtrs = Domain::ModelInstancePtrs;

// Set the print_volume_state of PrintObject::instances, return the total number of printable objects.
unsigned int update_print_volume_state(Model& model, const BuildVolume& build_volume);

inline void model_volumes_sort_by_id(ModelVolumePtrs &model_volumes)
{
    std::sort(model_volumes.begin(), model_volumes.end(), [](const ModelVolume *l, const ModelVolume *r) { return l->id() < r->id(); });
}

inline const ModelVolume* model_volume_find_by_id(const ModelVolumePtrs &model_volumes, const Domain::ObjectID id)
{
    auto it = lower_bound_by_predicate(model_volumes.begin(), model_volumes.end(), [id](const ModelVolume *mv) { return mv->id() < id; });
    return it != model_volumes.end() && (*it)->id() == id ? *it : nullptr;
}

// Test whether the two models contain the same number of ModelObjects with the same set of IDs
// ordered in the same order. In that case it is not necessary to kill the background processing.
bool model_object_list_equal(const ModelObjectPtrs &old_objects, const ModelObjectPtrs &new_objects);

// Test whether the new model is just an extension of the old model (new objects were added
// to the end of the original list. In that case it is not necessary to kill the background processing.
bool model_object_list_extended(const Model &model_old, const Model &model_new);

// Test whether the new ModelObject contains a different set of volumes (or sorted in a different order)
// than the old ModelObject.
bool model_volume_list_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, const ModelVolumeType type);
bool model_volume_list_changed(const ModelObject &model_object_old, const ModelObject &model_object_new, const std::initializer_list<ModelVolumeType> &types);

// Test whether the now ModelObject has newer custom supports data than the old one.
// The function assumes that volumes list is synchronized.
bool model_custom_supports_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// Test whether the now ModelObject has newer custom seam data than the old one.
// The function assumes that volumes list is synchronized.
bool model_custom_seam_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// Test whether the now ModelObject has newer MMU segmentation data than the old one.
// The function assumes that volumes list is synchronized.
extern bool model_mmu_segmentation_data_changed(const ModelObject& mo, const ModelObject& mo_new);

// Test whether the now ModelObject has newer fuzzy skin data than the old one.
// The function assumes that volumes list is synchronized.
extern bool model_fuzzy_skin_data_changed(const ModelObject &mo, const ModelObject &mo_new);

// If the model has object(s) which contains a modofoer, then it is currently not supported by the SLA mode.
// Either the model cannot be loaded, or a SLA printer has to be activated.
bool model_has_parameter_modifiers_in_objects(const Model& model);
// If the model has advanced features, then it cannot be processed in simple mode.
bool model_has_advanced_features(const Model &model);

#ifndef NDEBUG
// Verify whether the IDs of Model / ModelObject / ModelVolume / ModelInstance are valid and unique.
void check_model_ids_validity(const Model &model);
void check_model_ids_equal(const Model &model1, const Model &model2);
#endif /* NDEBUG */

} // namespace Slic3r

namespace cereal
{
    template <class Archive> struct specialize<Archive, Slic3r::Domain::VolumeSettings, cereal::specialization::member_serialize> {};
    template <class Archive> struct specialize<Archive, Slic3r::Domain::ObjectSettings, cereal::specialization::member_serialize> {};
    template <class Archive> struct specialize<Archive, Slic3r::Domain::SLAObjectSettings, cereal::specialization::member_serialize> {};
}

#endif /* slic3r_Model_hpp_ */
