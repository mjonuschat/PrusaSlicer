///|/ Copyright (c) Prusa Research 2016 - 2023 Tomáš Mészáros @tamasmeszaros, Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, Filip Sykala @Jony01, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2021 Boleslaw Ciesielski
///|/ Copyright (c) 2019 John Drake @foxox
///|/ Copyright (c) 2019 Sijmen Schoon
///|/ Copyright (c) Slic3r 2014 - 2016 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2015 Maksim Derbasov @ntfshard
///|/
///|/ ported from lib/Slic3r/Model.pm:
///|/ Copyright (c) Prusa Research 2016 - 2022 Vojtěch Bubník @bubnikv, Enrico Turri @enricoturri1966
///|/ Copyright (c) Slic3r 2012 - 2016 Alessandro Ranellucci @alranel
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "libslic3r/ModelProcessing.hpp"

#include "Slic3r/Biz/Algorithms/ModelObject.hpp"
#include "Slic3r/Biz/Algorithms/ModelVolume.hpp"
#include "Slic3r/Domain/Model.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

using namespace Slic3r::Biz;

namespace Slic3r::ModelProcessing {

namespace tm = Slic3r::Biz::Algorithms::TriangleMesh;
using Domain::TriangleMesh;
using Domain::TriangleMeshStats;
using Domain::RepairedMeshErrors;

// Generate next extruder ID string, in the range of (1, max_extruders).
static inline int auto_extruder_id(unsigned int max_extruders, unsigned int& cntr)
{
    int out = ++cntr;
    if (cntr == max_extruders)
        cntr = 0;
    return out;
}

void convert_to_multipart_object(Domain::Model& model, unsigned int max_extruders)
{
    assert(model.objects.size() >= 2);
    if (model.objects.size() < 2)
        return;

    Domain::Model tmp_model = Domain::Model();
    tmp_model.add_object();

    Domain::ModelObject* object = tmp_model.objects[0];
    object->input_file = model.objects.front()->input_file;
    object->name = boost::filesystem::path(model.objects.front()->input_file).stem().string();
    //FIXME copy the config etc?

    unsigned int extruder_counter = 0;
    for (const Domain::ModelObject* o : model.objects)
        for (const Domain::ModelVolume* v : o->volumes) {
            // If there are more than one object, put all volumes together 
            // Each object may contain any number of volumes and instances
            // The volumes transformations are relative to the object containing them...
            using Domain::Transformation;
            Transformation trafo_volume = v->get_transformation();
            // Revert the centering operation.
            trafo_volume.set_offset(trafo_volume.get_offset() - o->origin_translation);
            int counter = 1;
            auto copy_volume = [o, max_extruders, &counter, &extruder_counter](Domain::ModelVolume* new_v) {
                assert(new_v != nullptr);
                new_v->name = (counter > 1) ? o->name + "_" + std::to_string(counter++) : o->name;
                new_v->volume_settings.overrides.set("extruder", auto_extruder_id(max_extruders, extruder_counter));
                return new_v;
                };
            if (o->instances.empty()) {
                copy_volume(object->add_volume(*v))->set_transformation(trafo_volume);
            }
            else {
                for (const Domain::ModelInstance* i : o->instances)
                    // ...so, transform everything to a common reference system (world)
                    copy_volume(object->add_volume(*v))->set_transformation(i->get_transformation() * trafo_volume);
            }
        }

    // commented-out to fix #2868
//    object->add_instance();
//    object->instances[0]->set_offset(object->raw_mesh_bounding_box().center());

    model.clear_objects();
    model.add_object(*object);
}

void convert_from_imperial_units(Domain::Model& model, bool only_small_volumes)
{
    static constexpr const float in_to_mm = 25.4f;
    for (Domain::ModelObject* obj : model.objects)
        if (!only_small_volumes || get_object_mesh_stats(obj).volume < volume_threshold_inches) {
            obj->scale_mesh_after_creation(in_to_mm);
            for (Domain::ModelVolume* v : obj->volumes) {
                assert(!v->source.is_converted_from_meters);
                v->source.is_converted_from_inches = true;
            }
        }
}

void convert_from_imperial_units(Domain::ModelVolume* volume)
{
    assert(!volume->source.is_converted_from_meters);
    volume->scale_geometry_after_creation(25.4f);
    volume->set_offset(Domain::Vec3d(0, 0, 0));
    volume->source.is_converted_from_inches = true;
}

void convert_from_meters(Domain::Model& model, bool only_small_volumes)
{
    static constexpr const double m_to_mm = 1000;
    for (Domain::ModelObject* obj : model.objects)
        if (!only_small_volumes || get_object_mesh_stats(obj).volume < volume_threshold_meters) {
            obj->scale_mesh_after_creation(m_to_mm);
            for (Domain::ModelVolume* v : obj->volumes) {
                assert(!v->source.is_converted_from_inches);
                v->source.is_converted_from_meters = true;
            }
        }
}

void convert_from_meters(Domain::ModelVolume* volume)
{
    assert(!volume->source.is_converted_from_inches);
    volume->scale_geometry_after_creation(1000.f);
    volume->set_offset(Domain::Vec3d(0, 0, 0));
    volume->source.is_converted_from_meters = true;
}

void convert_units(Domain::Model& model_to, Domain::ModelObject* object_from, ConversionType conv_type, std::vector<int> volume_idxs)
{
    BOOST_LOG_TRIVIAL(trace) << "ModelObject::convert_units - start";

    float koef = conv_type == ConversionType::CONV_FROM_INCH ? 25.4f   : conv_type == ConversionType::CONV_TO_INCH ? 0.0393700787f :
                 conv_type == ConversionType::CONV_FROM_METER ? 1000.f : conv_type == ConversionType::CONV_TO_METER ? 0.001f : 1.f;

    Domain::ModelObject* new_object = model_to.add_object(*object_from);
    new_object->sla_support_points.clear();
    new_object->sla_drain_holes.clear();
    new_object->sla_points_status = Domain::SLA::PointsStatus::NoPoints;
    new_object->clear_volumes();
    new_object->input_file.clear();

    int vol_idx = 0;
    for (Domain::ModelVolume* volume : object_from->volumes) {
        if (!volume->mesh().empty()) {
            TriangleMesh mesh(volume->mesh());

            Domain::ModelVolume* vol = Algorithms::ModelObject::add_volume(new_object, mesh);
            vol->name = volume->name;
            vol->set_type(volume->type());
            vol->volume_settings = volume->volume_settings;
            vol->source.input_file = volume->source.input_file;
            vol->source.object_idx = (int)model_to.objects.size()-1;
            vol->source.volume_idx = vol_idx;
            vol->source.is_converted_from_inches = volume->source.is_converted_from_inches;
            vol->source.is_converted_from_meters = volume->source.is_converted_from_meters;
            vol->source.is_from_builtin_objects = volume->source.is_from_builtin_objects;

            vol->supported_facets.assign(volume->supported_facets);
            vol->seam_facets.assign(volume->seam_facets);
            vol->mm_segmentation_facets.assign(volume->mm_segmentation_facets);

            // Perform conversion only if the target "imperial" state is different from the current one.
            // This check supports conversion of "mixed" set of volumes, each with different "imperial" state.
            if (//vol->source.is_converted_from_inches != from_imperial && 
                (volume_idxs.empty() ||
                    std::find(volume_idxs.begin(), volume_idxs.end(), vol_idx) != volume_idxs.end())) {
                vol->scale_geometry_after_creation(koef);
                vol->set_offset(Domain::Vec3d(koef, koef, koef).cwiseProduct(volume->get_offset()));
                if (conv_type == ConversionType::CONV_FROM_INCH || conv_type == ConversionType::CONV_TO_INCH)
                    vol->source.is_converted_from_inches = conv_type == ConversionType::CONV_FROM_INCH;
                if (conv_type == ConversionType::CONV_FROM_METER || conv_type == ConversionType::CONV_TO_METER)
                    vol->source.is_converted_from_meters = conv_type == ConversionType::CONV_FROM_METER;
                assert(!vol->source.is_converted_from_inches || !vol->source.is_converted_from_meters);
            }
            else
                vol->set_offset(volume->get_offset());
        }
        vol_idx++;
    }
    new_object->invalidate_bounding_box();

    BOOST_LOG_TRIVIAL(trace) << "ModelObject::convert_units - end";
}

TriangleMeshStats get_object_mesh_stats(const Domain::ModelObject* object)
{
    TriangleMeshStats full_stats;
    full_stats.volume = 0.f;

    // fill full_stats from all objet's meshes
    for (Domain::ModelVolume* volume : object->volumes)
    {
        const TriangleMeshStats& stats = volume->mesh().stats();

        // initialize full_stats (for repaired errors)
        full_stats.open_edges += stats.open_edges;
        full_stats.repaired_errors.merge(stats.repaired_errors);

        // another used satistics value
        if (volume->is_model_part()) {
            Domain::Transform3d trans = object->instances.empty() ? volume->get_matrix() : (volume->get_matrix() * object->instances[0]->get_matrix());
            full_stats.volume += stats.volume * std::fabs(trans.matrix().block(0, 0, 3, 3).determinant());
            full_stats.number_of_parts += stats.number_of_parts;
        }
    }

    return full_stats;
}

int get_repaired_errors_count(const Domain::ModelVolume* volume)
{
    const RepairedMeshErrors& errors = volume->mesh().stats().repaired_errors;
    return  errors.degenerate_facets + 
            errors.edges_fixed + 
            errors.facets_removed +
            errors.facets_reversed + 
            errors.backwards_edges;
}

int get_repaired_errors_count(const Domain::ModelObject* object, const int vol_idx /*= -1*/)
{
    if (vol_idx >= 0)
        return get_repaired_errors_count(object->volumes[vol_idx]);

    const RepairedMeshErrors& errors = get_object_mesh_stats(object).repaired_errors;
    return  errors.degenerate_facets + 
            errors.edges_fixed + 
            errors.facets_removed +
            errors.facets_reversed + 
            errors.backwards_edges;
}



/// <summary>
/// Compare TriangleMeshes by Bounding boxes (mainly for sort)
/// From Front(Z) Upper(Y) TopLeft(X) corner.
/// 1. Seraparate group not overlaped i Z axis
/// 2. Seraparate group not overlaped i Y axis
/// 3. Start earlier in X (More on left side)
/// </summary>
/// <param name="triangle_mesh1">Compare from</param>
/// <param name="triangle_mesh2">Compare to</param>
/// <returns>True when triangle mesh 1 is closer, upper or lefter than triangle mesh 2 other wise false</returns>
static bool is_front_up_left(const TriangleMesh &trinagle_mesh1, const TriangleMesh &triangle_mesh2)
{
    // stats form t1
    const Domain::Vec3f &min1 = trinagle_mesh1.stats().min;
    const Domain::Vec3f &max1 = trinagle_mesh1.stats().max;
    // stats from t2
    const Domain::Vec3f &min2 = triangle_mesh2.stats().min;
    const Domain::Vec3f &max2 = triangle_mesh2.stats().max;
    // priority Z, Y, X
    for (int axe = 2; axe > 0; --axe) {
        if (max1[axe] < min2[axe])
            return true;
        if (min1[axe] > max2[axe])
            return false;
    }
    return min1.x() < min2.x();
}

template <typename ObjectSettingsType>
static ObjectSettingsType create_object_settings_from_volume_settings(const Domain::VolumeSettings &volume_settings)
{
    ObjectSettingsType object_settings;
    for (const Domain::ConfigItem& item : volume_settings.items.all_items()) {
        if (!volume_settings.overrides.get(item.name()).has_value() || object_settings.items.find(item.name()) == nullptr)
            continue;

        item.visit([&]<typename T>(const T& item_value) {
            using ValueType = std::decay_t<T>;
            object_settings.overrides.template set<ValueType>(item.name(), item_value);
        });
    }

    return object_settings;
}

void split(Domain::ModelObject* object, Domain::ModelObjectPtrs* new_objects)
{
    for (Domain::ModelVolume* volume : object->volumes) {
        if (volume->type() != Domain::ModelVolumeType::MODEL_PART)
            continue;

        // splited volume should not be text object 
        if (volume->text_configuration.has_value())
            volume->text_configuration.reset();

        std::vector<TriangleMesh> meshes = tm::split(volume->mesh());
        std::sort(meshes.begin(), meshes.end(), is_front_up_left);

        size_t counter = 1;
        for (TriangleMesh &mesh : meshes) {
            // FIXME: crashes if not satisfied
            if (mesh.facets_count() < 3 || mesh.has_zero_volume())
                continue;

            // XXX: this seems to be the only real usage of m_model, maybe refactor this so that it's not needed?
            Domain::ModelObject* new_object = object->get_model()->add_object();
            if (meshes.size() == 1) {
                new_object->name                = volume->name;
                new_object->object_settings     = object->object_settings.overrides.empty()     ? create_object_settings_from_volume_settings<Domain::ObjectSettings>(volume->volume_settings)    : object->object_settings;
                new_object->object_settings_sla = object->object_settings_sla.overrides.empty() ? create_object_settings_from_volume_settings<Domain::SLAObjectSettings>(volume->volume_settings) : object->object_settings_sla;
            }
            else {
                new_object->name                = object->name + (meshes.size() > 1 ? "_" + std::to_string(counter++) : "");
                new_object->object_settings     = object->object_settings;
                new_object->object_settings_sla = object->object_settings_sla;
            }

            new_object->instances.reserve(object->instances.size());
            for (const Domain::ModelInstance* model_instance : object->instances)
                new_object->add_instance(*model_instance);

            Domain::ModelVolume* new_vol = Algorithms::ModelObject::add_volume(new_object, *volume, std::move(mesh));

            // Invalidate extruder value in volume's config,
            // otherwise there will no way to change extruder for object after splitting,
            // because volume's extruder value overrides object's extruder value.
            if (new_vol->volume_settings.overrides.get("extruder").has_value()) {
                new_vol->volume_settings.overrides.set("extruder", 0);
            }

            for (Domain::ModelInstance* model_instance : new_object->instances) {
                const Domain::Vec3d shift = model_instance->get_transformation().get_matrix_no_offset() * new_vol->get_offset();
                model_instance->set_offset(model_instance->get_offset() + shift);
            }

            new_vol->set_offset(Domain::Vec3d::Zero());
            // reset the source to disable reload from disk
            new_vol->source = Domain::ModelVolume::Source();
            new_objects->emplace_back(new_object);
        }
    }
}

void merge(Domain::ModelObject* object)
{
    if (object->volumes.size() == 1) {
        // We can't merge meshes if there's just one volume
        return;
    }

    TriangleMesh mesh;

    for (Domain::ModelVolume* volume : object->volumes)
        if (!volume->mesh().empty())
            mesh.merge(volume->mesh());

    object->clear_volumes();
    Domain::ModelVolume* vol = Algorithms::ModelObject::add_volume(object, mesh);

    if (!vol)
        return;
}

}
