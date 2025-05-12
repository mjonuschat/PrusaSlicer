///|/ Copyright (c) Prusa Research 2018 - 2023 Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Pavel Mikuš @Godrak, Oleksandra Iushchenko @YuSanka, Vojtěch Bubník @bubnikv, Roman Beránek @zavorka, Enrico Turri @enricoturri1966
///|/ Copyright (c) 2022 ole00 @ole00
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "SLAPrint.hpp"
#include "SLAPrintSteps.hpp" // IWYU pragma: keep
#include "CSGMesh/CSGMeshCopy.hpp"
#include "CSGMesh/PerformCSGMeshBooleans.hpp"
#include "format.hpp"
#include "StaticMap.hpp"

#include "Format/SLAArchiveFormatRegistry.hpp"

#include "Geometry.hpp"
#include "Thread.hpp"

#include <unordered_set>
#include <numeric>

#include <tbb/parallel_for.h>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

#include "libslic3r/MultipleBeds.hpp"
#include "libslic3r/Utils.hpp"

#include <libslic3r/SLA/SLAResult.hpp>

#include <libslic3r/Format/SL1.hpp>
#include <boost/algorithm/string.hpp>

// #define SLAPRINT_DO_BENCHMARK

#ifdef SLAPRINT_DO_BENCHMARK
#include <libnest2d/tools/benchmark.h>
#endif

#include "I18N.hpp"

//! macro used to mark string used at localization,
//! return same string
#define _u8L(s) Slic3r::I18N::translate(s)

namespace Slic3r {

using Domain::SLA::PointsStatus;
using SLASlicingSync::AllSteps;
using SLASlicingSync::AllOrSome;
using SLASlicingSync::PrintSteps;
using SLASlicingSync::PrintObjectSteps;
using SLASlicingSync::StepsPerPrintObject;
using SLASlicingSync::PrintAndObjectSteps;
using SLASlicingSync::InvalidatedSteps;
using Domain::ObjectID;


bool is_zero_elevation(const SLAPrintObjectConfig &c)
{
    return c.pad_enable.getBool() && c.pad_around_object.getBool();
}

// Compile the argument for support creation from the static print config.
sla::SupportTreeConfig make_support_cfg(const SLAPrintObjectConfig& c)
{
    sla::SupportTreeConfig scfg;

    scfg.enabled = c.supports_enable.getBool();
    scfg.tree_type = c.support_tree_type.value;

    switch(scfg.tree_type) {
    case sla::SupportTreeType::Default: {
        scfg.head_front_radius_mm = 0.5*c.support_head_front_diameter.getFloat();
        double pillar_r = 0.5 * c.support_pillar_diameter.getFloat();
        scfg.head_back_radius_mm = pillar_r;
        scfg.head_fallback_radius_mm =
            0.01 * c.support_small_pillar_diameter_percent.getFloat() * pillar_r;
        scfg.head_penetration_mm = c.support_head_penetration.getFloat();
        scfg.head_width_mm = c.support_head_width.getFloat();
        scfg.object_elevation_mm = is_zero_elevation(c) ?
                                       0. : c.support_object_elevation.getFloat();
        scfg.bridge_slope = c.support_critical_angle.getFloat() * PI / 180.0 ;
        scfg.max_bridge_length_mm = c.support_max_bridge_length.getFloat();
        scfg.max_pillar_link_distance_mm = c.support_max_pillar_link_distance.getFloat();
        scfg.pillar_connection_mode = c.support_pillar_connection_mode.value;
        scfg.ground_facing_only = c.support_buildplate_only.getBool();
        scfg.pillar_widening_factor = c.support_pillar_widening_factor.getFloat();
        scfg.base_radius_mm = 0.5*c.support_base_diameter.getFloat();
        scfg.base_height_mm = c.support_base_height.getFloat();
        scfg.pillar_base_safety_distance_mm =
            c.support_base_safety_distance.getFloat() < EPSILON ?
                scfg.safety_distance_mm : c.support_base_safety_distance.getFloat();

        scfg.max_bridges_on_pillar = unsigned(c.support_max_bridges_on_pillar.getInt());
        scfg.max_weight_on_model_support = c.support_max_weight_on_model.getFloat();
        break;
    }
    case sla::SupportTreeType::Branching:
        [[fallthrough]];
    case sla::SupportTreeType::Organic:{
        scfg.head_front_radius_mm = 0.5*c.branchingsupport_head_front_diameter.getFloat();
        double pillar_r = 0.5 * c.branchingsupport_pillar_diameter.getFloat();
        scfg.head_back_radius_mm = pillar_r;
        scfg.head_fallback_radius_mm =
            0.01 * c.branchingsupport_small_pillar_diameter_percent.getFloat() * pillar_r;
        scfg.head_penetration_mm = c.branchingsupport_head_penetration.getFloat();
        scfg.head_width_mm = c.branchingsupport_head_width.getFloat();
        scfg.object_elevation_mm = is_zero_elevation(c) ?
                                       0. : c.branchingsupport_object_elevation.getFloat();
        scfg.bridge_slope = c.branchingsupport_critical_angle.getFloat() * PI / 180.0 ;
        scfg.max_bridge_length_mm = c.branchingsupport_max_bridge_length.getFloat();
        scfg.max_pillar_link_distance_mm = c.branchingsupport_max_pillar_link_distance.getFloat();
        scfg.pillar_connection_mode = c.branchingsupport_pillar_connection_mode.value;
        scfg.ground_facing_only = c.branchingsupport_buildplate_only.getBool();
        scfg.pillar_widening_factor = c.branchingsupport_pillar_widening_factor.getFloat();
        scfg.base_radius_mm = 0.5*c.branchingsupport_base_diameter.getFloat();
        scfg.base_height_mm = c.branchingsupport_base_height.getFloat();
        scfg.pillar_base_safety_distance_mm =
            c.branchingsupport_base_safety_distance.getFloat() < EPSILON ?
                scfg.safety_distance_mm : c.branchingsupport_base_safety_distance.getFloat();

        scfg.max_bridges_on_pillar = unsigned(c.branchingsupport_max_bridges_on_pillar.getInt());
        scfg.max_weight_on_model_support = c.branchingsupport_max_weight_on_model.getFloat();
        break;
    }
    }
    
    return scfg;
}

sla::PadConfig::EmbedObject builtin_pad_cfg(const SLAPrintObjectConfig& c)
{
    sla::PadConfig::EmbedObject ret;
    
    ret.enabled = is_zero_elevation(c);
    
    if(ret.enabled) {
        ret.everywhere           = c.pad_around_object_everywhere.getBool();
        ret.object_gap_mm        = c.pad_object_gap.getFloat();
        ret.stick_width_mm       = c.pad_object_connector_width.getFloat();
        ret.stick_stride_mm      = c.pad_object_connector_stride.getFloat();
        ret.stick_penetration_mm = c.pad_object_connector_penetration
                                       .getFloat();
    }
    
    return ret;
}

sla::PadConfig make_pad_cfg(const SLAPrintObjectConfig& c)
{
    sla::PadConfig pcfg;
    
    pcfg.wall_thickness_mm = c.pad_wall_thickness.getFloat();
    pcfg.wall_slope = c.pad_wall_slope.getFloat() * PI / 180.0;
    
    pcfg.max_merge_dist_mm = c.pad_max_merge_distance.getFloat();
    pcfg.wall_height_mm = c.pad_wall_height.getFloat();
    pcfg.brim_size_mm = c.pad_brim_size.getFloat();
    
    // set builtin pad implicitly ON
    pcfg.embed_object = builtin_pad_cfg(c);
    
    return pcfg;
}

bool validate_pad(const indexed_triangle_set &pad, const sla::PadConfig &pcfg)
{
    // An empty pad can only be created if embed_object mode is enabled
    // and the pad is not forced everywhere
    return !pad.empty() || (pcfg.embed_object.enabled && !pcfg.embed_object.everywhere);
}

SLAPrint::SLAPrint(const OnSlaResult& on_sla_result, const OnSlaObject& on_sla_object)
    : m_on_sla_result(on_sla_result)
    , m_on_sla_object(on_sla_object)
{}

void SLAPrint::clear()
{
    std::scoped_lock<std::mutex> lock(this->state_mutex());
    // The following call should stop background processing if it is running.
    this->invalidate_all_steps();
    for (SLAPrintObject *object : m_objects)
        delete object;
    m_objects.clear();
    m_model.clear_objects();
}

// Transformation without rotation around Z and without a shift by X and Y.
Transform3d SLAPrint::sla_trafo(const ModelObject &model_object) const
{
    ModelInstance &model_instance = *model_object.instances.front();
    auto trafo = Transform3d::Identity();
    trafo.translate(Vec3d{ 0., 0., model_instance.get_offset().z() * this->relative_correction().z() });
    trafo.linear() = Eigen::DiagonalMatrix<double, 3, 3>(this->relative_correction()) * model_instance.get_matrix().linear();
    if (model_instance.is_left_handed())
        trafo = Eigen::Scaling(Vec3d(-1., 1., 1.)) * trafo;
    return trafo;
}

namespace {
// Transformation without rotation around Z and without a shift by X and Y.
Transform3d sla_trafo(const ModelObject &model_object, const Vec3d& relative_correction)
{
    ModelInstance &model_instance = *model_object.instances.front();
    auto trafo = Transform3d::Identity();
    trafo.translate(Vec3d{ 0., 0., model_instance.get_offset().z() * relative_correction.z() });
    trafo.linear() = Eigen::DiagonalMatrix<double, 3, 3>(relative_correction) * model_instance.get_matrix().linear();
    if (model_instance.is_left_handed())
        trafo = Eigen::Scaling(Vec3d(-1., 1., 1.)) * trafo;
    return trafo;
}
}

// List of instances, where the ModelInstance transformation is a composite of sla_trafo and the transformation defined by SLAPrintObject::Instance.
static std::vector<SLAPrintObject::Instance> sla_instances(const ModelObject &model_object)
{
    std::vector<SLAPrintObject::Instance> instances;
    assert(! model_object.instances.empty());
    if (! model_object.instances.empty()) {
        const Transform3d& trafo0 = model_object.instances.front()->get_matrix();
        for (ModelInstance *model_instance : model_object.instances)
            if (model_instance->is_printable()) {
                instances.emplace_back(
                    model_instance->id(),
                    scaled(Vec2d(model_instance->get_offset(X), model_instance->get_offset(Y))),
                    float(Geometry::rotation_diff_z(trafo0, model_instance->get_matrix())));
            }
    }
    return instances;
}

std::vector<Domain::ObjectID> SLAPrint::print_object_ids() const
{ 
    std::vector<Domain::ObjectID> out;
    // Reserve one more for the caller to append the ID of the Print itself.
    out.reserve(m_objects.size() + 1);
    for (const SLAPrintObject *print_object : m_objects)
        out.emplace_back(print_object->id());
    return out;
}

static t_config_option_keys print_config_diffs(const StaticPrintConfig     &current_config,
                                               const DynamicPrintConfig &new_full_config,
                                               DynamicPrintConfig       &material_overrides)
{
    using namespace std::string_view_literals;

    static const constexpr StaticSet overriden_keys = {
        "support_head_front_diameter"sv,
        "support_head_penetration"sv,
        "support_head_width"sv,
        "support_pillar_diameter"sv,
        "branchingsupport_head_front_diameter"sv,
        "branchingsupport_head_penetration"sv,
        "branchingsupport_head_width"sv,
        "branchingsupport_pillar_diameter"sv,
        "support_points_density_relative"sv,
        "elefant_foot_compensation"sv,
        "absolute_correction"sv,
    };

    static constexpr auto material_ow_prefix = "material_ow_";

    t_config_option_keys           print_diff;
    for (const t_config_option_key &opt_key : current_config.keys()) {
        const ConfigOption *opt_old = current_config.option(opt_key);
        assert(opt_old != nullptr);
        const ConfigOption *opt_new = new_full_config.option(opt_key);
        // assert(opt_new != nullptr);
        if (opt_new == nullptr)
            //FIXME This may happen when executing some test cases.
            continue;
        const ConfigOption *opt_new_override = std::binary_search(overriden_keys.begin(), overriden_keys.end(), opt_key) ? new_full_config.option(material_ow_prefix + opt_key) : nullptr;
        if (opt_new_override != nullptr && ! opt_new_override->is_nil()) {
            // An override is available at some of the material presets.
            bool overriden = opt_new->overriden_by(opt_new_override);
            if (overriden || *opt_old != *opt_new) {
                auto opt_copy = opt_new->clone();
                opt_copy->apply_override(opt_new_override);
                bool changed = *opt_old != *opt_copy;
                if (changed)
                    print_diff.emplace_back(opt_key);
                if (changed || overriden) {
                    // overrides will be applied to the placeholder parser, which layers these parameters over full_print_config.
                    material_overrides.set_key_value(opt_key, opt_copy);
                } else
                    delete opt_copy;
            }
        } else if (*opt_new != *opt_old)
            print_diff.emplace_back(opt_key);
    }

    return print_diff;
}

namespace {

void update_placeholder_parser(PlaceholderParser& parser, const DynamicPrintConfig& config)
{
    const std::vector<std::string> placeholder_parser_diff{parser.config_diff(config)};
    // Apply variables to placeholder parser. The placeholder parser is currently used
    // only to generate the output file name.
    if (!placeholder_parser_diff.empty()) {
        // update_apply_status(this->invalidate_step(slapsRasterize));
        parser.apply_config(config);
        // Set the profile aliases for the PrintBase::output_filename()
        parser.set("print_preset", config.option("sla_print_settings_id")->clone());
        parser.set("material_preset", config.option("sla_material_settings_id")->clone());
        parser.set("printer_preset", config.option("printer_settings_id")->clone());
        parser.set("physical_printer_preset", config.option("physical_printer_settings_id")->clone());
    }
}

using StepsPerModelObjectId = std::map<ObjectID, std::set<SLAPrintObjectStep>>;

template <typename Set>
AllOrSome<Set> merge (const AllOrSome<Set>& a, const AllOrSome<Set>& b) {
    if (std::holds_alternative<AllSteps>(a) || std::holds_alternative<AllSteps>(b)) {
        return AllSteps{};
    }

    Set values_a{std::get<Set>(a)};
    Set values_b{std::get<Set>(b)};
    values_a.merge(values_b);
    return values_a;
}
template AllOrSome<PrintSteps> merge(const AllOrSome<PrintSteps>& a, const AllOrSome<PrintSteps>& b);
template AllOrSome<PrintObjectSteps> merge(const AllOrSome<PrintObjectSteps>& a, const AllOrSome<PrintObjectSteps>& b);

template <typename Set>
AllOrSome<Set> merge(const std::vector<AllOrSome<Set>>& steps) {
    AllOrSome<Set> result;
    for (const AllOrSome<Set>& set : steps) {
        result = merge(result, set);
    }
    return result;
}

StepsPerPrintObject merge(const StepsPerPrintObject& a, const StepsPerPrintObject& b) {
    StepsPerPrintObject result{a};
    StepsPerPrintObject same_key_elements{b};

    result.merge(same_key_elements);

    for (const auto& [print_object, invalidated_steps] : same_key_elements) {
        result.at(print_object) = merge(result.at(print_object), invalidated_steps);
    }

    return result;
}

InvalidatedSteps merge(const InvalidatedSteps& a, const InvalidatedSteps& b) {
    return {merge(a.print, b.print), merge(a.object, b.object)};
}


InvalidatedSteps merge(const std::vector<InvalidatedSteps>& invalidated_steps) {
    InvalidatedSteps result;
    for (const InvalidatedSteps& steps : invalidated_steps) {
        result = merge(result, steps);
    }
    return result;
}


struct ModelObjectsSyncResult {
    ModelObjectPtrs objects;
    std::map<ObjectID, SLAPrintObject*> reuse_candidates;
    InvalidatedSteps invalidated_steps;
};

ModelObjectsSyncResult sync_model_objects(
    const ModelObjectPtrs& new_objects,
    const std::map<ObjectID, ModelObject*>& reuse_candidates,
    const PrintObjects& old_objects,
    const Vec3d& relative_correction
)
{
    ModelObjectPtrs objects;
    StepsPerModelObjectId reused_objects;

    for (ModelObject* model_object_new : new_objects) {
        const auto current_object_it{reuse_candidates.find(model_object_new->id())};

        if (current_object_it == reuse_candidates.end()) {
            objects.push_back(ModelObject::new_copy(*model_object_new));
            continue;
        }

        ModelObject* model_object{current_object_it->second};

        // Check whether a model part volume was added or removed, their transformations or order changed.
        const bool model_parts_differ{model_volume_list_changed(
            *model_object,
            *model_object_new,
            {ModelVolumeType::MODEL_PART,
             ModelVolumeType::NEGATIVE_VOLUME,
             ModelVolumeType::SUPPORT_ENFORCER,
             ModelVolumeType::SUPPORT_BLOCKER}
        )};

        const bool sla_trafo_differs = model_object->instances.empty() != model_object_new->instances.empty()
            || (!model_object->instances.empty() && (!sla_trafo(*model_object, relative_correction).isApprox(sla_trafo(*model_object_new, relative_correction))
            || model_object->instances.front()->is_left_handed() != model_object_new->instances.front()->is_left_handed()));

        if (model_parts_differ || sla_trafo_differs) {
            objects.push_back(ModelObject::new_copy(*model_object_new));
            continue;
        }

        std::set<SLAPrintObjectStep> invalidated_steps;

        const bool old_user_modified = model_object->sla_points_status
            == PointsStatus::UserModified;
        const bool new_user_modified = model_object_new->sla_points_status
            == PointsStatus::UserModified;

        const bool supports_equal{
            model_object->sla_support_points == model_object_new->sla_support_points
        };

        const bool switching_to_auto_from_man{old_user_modified && !new_user_modified};
        const bool switching_to_man_from_auto{!old_user_modified && new_user_modified};

        if (switching_to_auto_from_man
            || switching_to_man_from_auto
            || (new_user_modified && !supports_equal)) {
            invalidated_steps.insert(slaposSupportPoints);
        }

        // Invalidate hollowing if drain holes have changed
        if (model_object->sla_drain_holes != model_object_new->sla_drain_holes)
        {
            invalidated_steps.insert(slaposDrillHoles);
        }

        model_object->assign_copy(*model_object_new);

        objects.push_back(model_object);
        reused_objects.insert({model_object->id(), invalidated_steps});
    }

    ModelObjectsSyncResult result;
    result.objects = objects;

    for (SLAPrintObject* print_object : old_objects) {
        const auto reused_model_object_it{
            reused_objects.find(print_object->model_object()->id())
        };

        if (reused_model_object_it != reused_objects.end()) {
            result.invalidated_steps.object[print_object] = reused_model_object_it->second;
            result.reuse_candidates.insert({print_object->model_object()->id(), print_object});
        }
    }

    return result;
}

void delete_old_model_objects(const ModelObjectPtrs& old_objects, const ModelObjectPtrs& new_objects) {
    const std::set<ModelObject*> model_objects_set{
        new_objects.begin(),
        new_objects.end()
    };
    for (ModelObject* model_object : old_objects) {
        if (model_objects_set.count(model_object) == 0) {
            delete model_object;
        }
    }
}

AllOrSome<PrintSteps> get_steps_invalidated_by_config_options(const std::vector<t_config_option_key> &opt_keys)
{
    using namespace std::string_view_literals;

    if (opt_keys.empty())
        return PrintSteps{};

    static constexpr StaticSet steps_full = {
        "initial_layer_height"sv,
        "material_correction"sv,
        "material_correction_x"sv,
        "material_correction_y"sv,
        "material_correction_z"sv,
        "material_print_speed"sv,
        "relative_correction"sv,
        "relative_correction_x"sv,
        "relative_correction_y"sv,
        "relative_correction_z"sv,
        "absolute_correction"sv,
        "elefant_foot_compensation"sv,
        "elefant_foot_min_width"sv,
        "zcorrection_layers"sv,
        "gamma_correction"sv,
    };

    // Cache the plenty of parameters, which influence the final rasterization only,
    // or they are only notes not influencing the rasterization step.
    static constexpr StaticSet steps_rasterize = {
        "min_exposure_time"sv,
        "max_exposure_time"sv,
        "exposure_time"sv,
        "min_initial_exposure_time"sv,
        "max_initial_exposure_time"sv,
        "initial_exposure_time"sv,
        "display_width"sv,
        "display_height"sv,
        "display_pixels_x"sv,
        "display_pixels_y"sv,
        "display_mirror_x"sv,
        "display_mirror_y"sv,
        "display_orientation"sv,
        "sla_archive_format"sv,
        "sla_output_precision"sv,
        // tilt params
        "delay_before_exposure"sv,
        "delay_after_exposure"sv,
        "tower_hop_height"sv,
        "tower_speed"sv,
        "use_tilt"sv,
        "tilt_down_initial_speed"sv,
        "tilt_down_offset_steps"sv,
        "tilt_down_offset_delay"sv,
        "tilt_down_finish_speed"sv,
        "tilt_down_cycles"sv,
        "tilt_down_delay"sv,
        "tilt_up_initial_speed"sv,
        "tilt_up_offset_steps"sv,
        "tilt_up_offset_delay"sv,
        "tilt_up_finish_speed"sv,
        "tilt_up_cycles"sv,
        "tilt_up_delay"sv,
        "area_fill"sv,
    };

    static StaticSet steps_ignore = {
        "bed_shape"sv,
        "max_print_height"sv,
        "printer_technology"sv,
        "output_filename_format"sv,
        "fast_tilt_time"sv,
        "slow_tilt_time"sv,
        "high_viscosity_tilt_time"sv,
        "bottle_cost"sv,
        "bottle_volume"sv,
        "bottle_weight"sv,
        "material_density"sv,
        "material_ow_support_pillar_diameter"sv,
        "material_ow_support_head_front_diameter"sv,
        "material_ow_support_head_penetration"sv,
        "material_ow_support_head_width"sv,
        "material_ow_branchingsupport_pillar_diameter"sv,
        "material_ow_branchingsupport_head_front_diameter"sv,
        "material_ow_branchingsupport_head_penetration"sv,
        "material_ow_branchingsupport_head_width"sv,
        "material_ow_elefant_foot_compensation"sv,
        "material_ow_support_points_density_relative"sv,
        "material_ow_absolute_correction"sv,
        "printer_model"sv,
    };

    std::set<SLAPrintStep> steps;

    for (std::string_view opt_key : opt_keys) {
        if (steps_rasterize.find(opt_key) != steps_rasterize.end()) {
            // These options only affect the final rasterization, or they are just notes without influence on the output,
            // so there is nothing to invalidate.
            steps.insert(slapsMergeSlicesAndEval);
        } else if (steps_ignore.find(opt_key) != steps_ignore.end()) {
            // These steps have no influence on the output. Just ignore them.
        } else if (steps_full.find(opt_key) != steps_full.end()) {
            return AllSteps{};
        } else {
            // All values should be covered.
            assert(false);
        }
    }

    return steps;
}

PrintObjectSteps get_object_steps_invalidated_by_config_options(const std::vector<t_config_option_key> &opt_keys)
{
    if (opt_keys.empty()) {
        return {};
    }

    PrintObjectSteps result;
    for (const t_config_option_key &opt_key : opt_keys) {
        if (   opt_key == "hollowing_enable"
            || opt_key == "hollowing_min_thickness"
            || opt_key == "hollowing_quality"
            || opt_key == "hollowing_closing_distance"
            ) {
            result.insert(slaposHollowing);
        } else if (
               opt_key == "layer_height"
            || opt_key == "faded_layers"
            || opt_key == "pad_enable"
            || opt_key == "pad_wall_thickness"
            || opt_key == "supports_enable"
            || opt_key == "support_tree_type"
            || opt_key == "support_object_elevation"
            || opt_key == "branchingsupport_object_elevation"
            || opt_key == "pad_around_object"
            || opt_key == "pad_around_object_everywhere"
            || opt_key == "slice_closing_radius"
            || opt_key == "slicing_mode") {
            result.insert(slaposObjectSlice);
        } else if (
               opt_key == "support_points_density_relative"
            || opt_key == "support_enforcers_only"
            ) {
            result.insert(slaposSupportPoints);
        } else if (
               opt_key == "support_head_front_diameter"
            || opt_key == "support_head_penetration"
            || opt_key == "support_head_width"
            || opt_key == "support_pillar_diameter"
            || opt_key == "support_pillar_widening_factor"
            || opt_key == "support_small_pillar_diameter_percent"
            || opt_key == "support_max_weight_on_model"
            || opt_key == "support_max_bridges_on_pillar"
            || opt_key == "support_pillar_connection_mode"
            || opt_key == "support_buildplate_only"
            || opt_key == "support_base_diameter"
            || opt_key == "support_base_height"
            || opt_key == "support_critical_angle"
            || opt_key == "support_max_bridge_length"
            || opt_key == "support_max_pillar_link_distance"
            || opt_key == "support_base_safety_distance"
            || opt_key == "branchingsupport_head_front_diameter"
            || opt_key == "branchingsupport_head_penetration"
            || opt_key == "branchingsupport_head_width"
            || opt_key == "branchingsupport_pillar_diameter"
            || opt_key == "branchingsupport_pillar_widening_factor"
            || opt_key == "branchingsupport_small_pillar_diameter_percent"
            || opt_key == "branchingsupport_max_weight_on_model"
            || opt_key == "branchingsupport_max_bridges_on_pillar"
            || opt_key == "branchingsupport_pillar_connection_mode"
            || opt_key == "branchingsupport_buildplate_only"
            || opt_key == "branchingsupport_base_diameter"
            || opt_key == "branchingsupport_base_height"
            || opt_key == "branchingsupport_critical_angle"
            || opt_key == "branchingsupport_max_bridge_length"
            || opt_key == "branchingsupport_max_pillar_link_distance"
            || opt_key == "branchingsupport_base_safety_distance"
            || opt_key == "pad_object_gap"
            ) {
            result.insert(slaposSupportTree);
        } else if (
               opt_key == "pad_wall_height"
            || opt_key == "pad_brim_size"
            || opt_key == "pad_max_merge_distance"
            || opt_key == "pad_wall_slope"
            || opt_key == "pad_edge_radius"
            || opt_key == "pad_object_connector_stride"
            || opt_key == "pad_object_connector_width"
            || opt_key == "pad_object_connector_penetration"
            ) {
            result.insert(slaposPad);
        } else {
            // All keys should be covered.
            assert(false);
        }
    }

    return result;
}

void delete_old_print_objects(const PrintObjects& old_objects, const PrintObjects& new_objects)
{
    std::set<SLAPrintObject*> print_objects_set{new_objects.begin(), new_objects.end()};
    for (SLAPrintObject* print_object : old_objects) {
        if (print_objects_set.count(print_object) == 0) {
            delete print_object;
        }
    }
}

struct PrintObjectsSyncResult {
    std::vector<SLAPrintObject*> objects;
    InvalidatedSteps invalidated_steps;
};

PrintObjectsSyncResult sync_print_objects(
    const ModelObjectPtrs& model_objects,
    const std::map<ObjectID, SLAPrintObject*>& reuse_candidates,
    const SLAPrintObjectConfig& default_config,
    const Vec3d relative_correction,
    SLAPrint* print
)
{
    PrintObjectsSyncResult result;

    for (ModelObject* model_object: model_objects) {
        std::vector<SLAPrintObject::Instance> new_instances = sla_instances(*model_object);
        if (new_instances.empty()) {
            continue;
        }

        const auto it{reuse_candidates.find(model_object->id())};

        if (it == reuse_candidates.end()) {
            auto print_object = new SLAPrintObject(print, model_object);

            // FIXME: this invalidates the transformed mesh in SLAPrintObject
            // which is expensive to calculate (especially the raw_mesh() call)
            print_object->set_trafo(sla_trafo(*model_object, relative_correction), model_object->instances.front()->is_left_handed());

            print_object->set_instances(std::move(new_instances));

            print_object->config_apply(default_config, true);
            print_object->config_apply(model_object->config.get(), true);
            result.objects.emplace_back(print_object);
            continue;
        }

        SLAPrintObject* reused_print_object{it->second};

        // Synchronize Object's config.
        SLAPrintObjectConfig new_config = default_config;
        new_config.apply(model_object->config.get(), true);
        t_config_option_keys diff = reused_print_object->config().diff(new_config);
        if (! diff.empty()) {
            AllOrSome<PrintObjectSteps>& steps{
                result.invalidated_steps.object[reused_print_object]
            };
            if (std::holds_alternative<PrintObjectSteps>(steps)) {
                auto& object_steps{std::get<PrintObjectSteps>(steps)};
                object_steps.merge(get_object_steps_invalidated_by_config_options(diff));
            }
            reused_print_object->config_apply_only(new_config, diff, true);
        }

        if (new_instances != reused_print_object->instances()) {
            // Instances changed.
            reused_print_object->set_instances(std::move(new_instances));

            if (std::holds_alternative<PrintSteps>(result.invalidated_steps.print)) {
                auto& print_steps{std::get<PrintSteps>(result.invalidated_steps.print)};
                print_steps.insert(slapsMergeSlicesAndEval);
            }
        }
        result.objects.emplace_back(reused_print_object);
    }

    return result;
}

} // namespace

bool SLAPrint::invalidate_object_steps(
    const InvalidatedSteps& steps
) {
    bool invalidated{false};

    if (std::holds_alternative<PrintSteps>(steps.print)) {
        const auto print_steps{std::get<PrintSteps>(steps.print)};
        for (const SLAPrintStep& step : print_steps) {
            if (this->invalidate_step(step)) {
                invalidated = true;
            }
        }
    } else {
        if (this->invalidate_all_steps()) {
            invalidated = true;
        }
    }

    for (const auto& [print_object, invalidated_steps] : steps.object) {
        if (std::holds_alternative<PrintObjectSteps>(invalidated_steps)) {
            const auto object_steps{std::get<PrintObjectSteps>(invalidated_steps)};
            for (const SLAPrintObjectStep& step : object_steps) {
                if (print_object->invalidate_step(step)) {
                    invalidated = true;
                }
            }
        } else {
            if (print_object->invalidate_all_steps()) {
                invalidated = true;
            }
        }
    }

    return invalidated;
}

bool InvalidatedSteps::empty() const {
    if (std::holds_alternative<AllSteps>(print)) {
        return false;
    }
    if (!std::get<PrintSteps>(print).empty()) {
        return false;
    }

    for (const auto& [_, steps] : object) {
        if (std::holds_alternative<AllSteps>(steps)) {
            return false;
        }
        if (!std::get<PrintObjectSteps>(steps).empty()) {
            return false;
        }
    }
    return true;
}

struct ModelSyncResult {
    ModelObjectPtrs model_objects;
    PrintObjects print_objects;
    InvalidatedSteps invalidated_steps;
};

ModelSyncResult sync_model(
    const Model& old_model,
    const Model& new_model,
    const SLAPrintObjectConfig& default_object_config,
    const PrintObjects& old_objects,
    const std::map<ObjectID, ModelObject*>& reuse_candidates,
    const Vec3d& relative_correction,
    SLAPrint* print
)
{
    const ModelObjectsSyncResult model_objects_sync_result{
        sync_model_objects(new_model.objects, reuse_candidates, old_objects, relative_correction)
    };

    InvalidatedSteps changed_model_invalidated_steps;
    if (!model_object_list_equal(old_model.objects, model_objects_sync_result.objects)) {
        changed_model_invalidated_steps.print = PrintSteps{slapsMergeSlicesAndEval};
    }

    const PrintObjectsSyncResult print_objects_sync_result{
        sync_print_objects(
            model_objects_sync_result.objects,
            model_objects_sync_result.reuse_candidates,
            default_object_config,
            relative_correction,
            print
        )
    };

    delete_old_model_objects(old_model.objects, model_objects_sync_result.objects);
    delete_old_print_objects(old_objects, print_objects_sync_result.objects);

    InvalidatedSteps changed_objects_invalidated_steps;
    if (old_objects != print_objects_sync_result.objects) {
        changed_objects_invalidated_steps.print = AllSteps{};
    }

    return {
        model_objects_sync_result.objects,
        print_objects_sync_result.objects,
        merge({
            model_objects_sync_result.invalidated_steps,
            print_objects_sync_result.invalidated_steps,
            changed_model_invalidated_steps,
            changed_objects_invalidated_steps
        })
    };
}

SLAPrint::ApplyStatus SLAPrint::apply(
    const Model& model,
    DynamicPrintConfig config,
    const std::optional<Domain::ModelWipeTower>&,
    const std::optional<Domain::CustomGCode::Info>&,
    std::vector<std::string>* warnings
)
{
    this->call_cancel_callback();
#ifdef _DEBUG
    check_model_ids_validity(model);
#endif /* _DEBUG */

    // Normalize the config.
    config.option("sla_print_settings_id",        true);
    config.option("sla_material_settings_id",     true);
    config.option("printer_settings_id",          true);
    config.option("physical_printer_settings_id", true);
    // Collect changes to print config.
    DynamicPrintConfig mat_overrides;
    t_config_option_keys print_diff    = m_print_config.diff(config);
    t_config_option_keys printer_diff  = print_config_diffs(m_printer_config, config, mat_overrides);
    t_config_option_keys material_diff = m_material_config.diff(config);
    t_config_option_keys object_diff   = print_config_diffs(m_default_object_config, config, mat_overrides);

    config.apply(mat_overrides, true);

    // Grab the lock for the Print / PrintObject milestones.
    std::scoped_lock<std::mutex> lock(this->state_mutex());

    const InvalidatedSteps config_invalidated_steps{
        merge(
            std::vector<AllOrSome<PrintSteps>>{
                get_steps_invalidated_by_config_options(print_diff),
                get_steps_invalidated_by_config_options(printer_diff),
                get_steps_invalidated_by_config_options(material_diff)
            }
        ),
        {}
    };

    update_placeholder_parser(m_placeholder_parser, config);

    // It is also safe to change m_config now after this->invalidate_state_by_config_options() call.
    m_print_config.apply_only(config, print_diff, true);
    m_printer_config.apply_only(config, printer_diff, true);
    // Handle changes to material config.
    m_material_config.apply_only(config, material_diff, true);
    // Handle changes to object config defaults
    m_default_object_config.apply_only(config, object_diff, true);

    const bool all_invalidated{std::holds_alternative<AllSteps>(config_invalidated_steps.print)};

    std::map<ObjectID, ModelObject*> reuse_candidates;
    if (model.id() == m_model.id() && !all_invalidated) {
        for (ModelObject* model_object: m_model.objects) {
            reuse_candidates.insert({model_object->id(), model_object});
        }
    }

    m_model.copy_id(model);

    const ModelSyncResult model_sync_result{sync_model(
        m_model,
        model,
        m_default_object_config,
        m_objects,
        reuse_candidates,
        this->relative_correction(),
        this
    )};

    const InvalidatedSteps invalidated_steps{merge({
        config_invalidated_steps,
        model_sync_result.invalidated_steps
    })};

    m_model.objects = model_sync_result.model_objects;
    m_objects = model_sync_result.print_objects;

    if(m_objects.empty()) {
        m_printer_input = {};
    }

    m_full_print_config = std::move(config);

    const bool changed{!invalidated_steps.empty()};
    const bool invalidated{this->invalidate_object_steps(invalidated_steps)};

    if (invalidated) {
        return APPLY_STATUS_INVALIDATED;
    }
    if (changed) {
        return APPLY_STATUS_CHANGED;
    }
    return APPLY_STATUS_UNCHANGED;
}

namespace {
using namespace Slic3r::Biz::Slicing; //Sla::PrintStatistics
DynamicConfig to_config(const Sla::PrintStatistics& stats)
{
    DynamicConfig config;
    const std::string print_time = Slic3r::short_time(get_time_dhms(float(stats.estimated_print_time)));
    config.set_key_value("print_time", new ConfigOptionString(print_time));
    config.set_key_value("objects_used_material", new ConfigOptionFloat(stats.objects_used_material));
    config.set_key_value("support_used_material", new ConfigOptionFloat(stats.support_used_material));
    config.set_key_value("total_cost", new ConfigOptionFloat(stats.total_cost));
    config.set_key_value("total_weight", new ConfigOptionFloat(stats.total_weight));
    return config;
}

DynamicConfig create_stats_placeholders()
{
    DynamicConfig config;
    for (const char* key : {"print_time", "total_cost", "total_weight", 
        "objects_used_material", "support_used_material"})
        config.set_key_value(key, new ConfigOptionString(std::string("{") + key + "}"));
    return config;
}

} // namespace

// Generate a recommended output file name based on the format template, default extension, and template parameters
// (timestamps, object placeholders derived from the model, current placeholder prameters and print statistics.
// Use the final print statistics if available, or just keep the print statistics placeholders if not available yet (before the output is finalized).
std::string SLAPrint::output_filename(const std::string &filename_base) const
{
    DynamicConfig config = this->finished() ? to_config(m_print_statistics) : create_stats_placeholders();
    std::string default_ext = get_default_extension(m_printer_config.sla_archive_format.value.c_str());
    if (default_ext.empty())
        default_ext = "sl1";

    default_ext.insert(default_ext.begin(), '.');

    config.set_key_value("default_output_extension",
                         new ConfigOptionString(default_ext));

    return this->PrintBase::output_filename(m_print_config.output_filename_format.value, default_ext, filename_base, &config);
}

std::string SLAPrint::validate(std::vector<std::string>*) const
{
    for(SLAPrintObject * po : m_objects) {

        const ModelObject *mo = po->model_object();
        bool supports_en = po->config().supports_enable.getBool();

        if(supports_en &&
           mo->sla_points_status == PointsStatus::UserModified &&
           mo->sla_support_points.empty())
            return _u8L("Cannot proceed without support points! "
                     "Add support points or disable support generation.");

        sla::SupportTreeConfig cfg = make_support_cfg(po->config());

        double elv = cfg.object_elevation_mm;
        
        sla::PadConfig padcfg = make_pad_cfg(po->config());
        sla::PadConfig::EmbedObject &builtinpad = padcfg.embed_object;
        
        if(supports_en && !builtinpad.enabled && elv < cfg.head_fullwidth())
            return _u8L(
                "Elevation is too low for object. Use the \"Pad around "
                "object\" feature to print the object without elevation.");
        
        if(supports_en && builtinpad.enabled &&
           cfg.pillar_base_safety_distance_mm < builtinpad.object_gap_mm) {
            return _u8L(
                "The endings of the support pillars will be deployed on the "
                "gap between the object and the pad. 'Support base safety "
                "distance' has to be greater than the 'Pad object gap' "
                "parameter to avoid this.");
        }
        
        std::string pval = padcfg.validate();
        if (!pval.empty()) return pval;
    }

    double expt_max = m_printer_config.max_exposure_time.getFloat();
    double expt_min = m_printer_config.min_exposure_time.getFloat();
    double expt_cur = m_material_config.exposure_time.getFloat();

    if (expt_cur < expt_min || expt_cur > expt_max)
        return _u8L("Exposition time is out of printer profile bounds.");

    double iexpt_max = m_printer_config.max_initial_exposure_time.getFloat();
    double iexpt_min = m_printer_config.min_initial_exposure_time.getFloat();
    double iexpt_cur = m_material_config.initial_exposure_time.getFloat();

    if (iexpt_cur < iexpt_min || iexpt_cur > iexpt_max)
        return _u8L("Initial exposition time is out of printer profile bounds.");

    for (const std::string& prefix : { "", "branching" }) {

        double head_penetration = m_full_print_config.opt_float(prefix + "support_head_penetration");
        double head_width       = m_full_print_config.opt_float(prefix + "support_head_width");

        if (head_penetration > head_width) {
            return _u8L("Invalid Head penetration\n"
                        "Head penetration should not be greater than the Head width.\n"
                        "Please check value of Head penetration in Print Settings or Material Overrides.");
        }

        double pinhead_d = m_full_print_config.opt_float(prefix + "support_head_front_diameter");
        double pillar_d  = m_full_print_config.opt_float(prefix + "support_pillar_diameter");

        if (pinhead_d > pillar_d) {
            return _u8L("Invalid pinhead diameter\n"
                        "Pinhead front diameter should be smaller than the Pillar diameter.\n"
                        "Please check value of Pinhead front diameter in Print Settings or Material Overrides.");
        }
    }

    if ((!m_material_config.use_tilt.get_at(0) && is_approx(m_material_config.tower_hop_height.get_at(0), 0.))
        || (!m_material_config.use_tilt.get_at(1) && is_approx(m_material_config.tower_hop_height.get_at(1), 0.)))
        return _u8L("Disabling the 'Use tilt' function causes the object to separate away from the film in the "
                    "vertical direction only. Therefore, it is necessary to set the 'Tower hop height' parameter "
                    "to reasonable value. The recommended value is 5 mm.");

    return "";
}

bool SLAPrint::is_prusa_print(const std::string& printer_model)
{
    static const std::vector<std::string> prusa_printer_models = { "SL1", "SL1S", "M1", "SLX" };
    for (const std::string& model : prusa_printer_models)
        if (model == printer_model)
            return true;

    return false;
}

bool SLAPrint::invalidate_step(SLAPrintStep step)
{
    bool invalidated = Inherited::invalidate_step(step);

    // propagate to dependent steps
    if (step == slapsMergeSlicesAndEval) {
        invalidated |= this->invalidate_all_steps();
    }

    return invalidated;
}

void SLAPrint::process()
{
    if (m_objects.empty())
        return;

    name_tbb_thread_pool_threads_set_locale();

    // Assumption: at this point the print objects should be populated only with
    // the model objects we have to process and the instances are also filtered
    
    Steps printsteps(this);

    // We want to first process all objects...
    std::vector<SLAPrintObjectStep> level1_obj_steps = {
        slaposAssembly, slaposHollowing, slaposDrillHoles, slaposObjectSlice, slaposSupportPoints, slaposSupportTree, slaposPad
    };

    // and then slice all supports to allow preview to be displayed ASAP
    std::vector<SLAPrintObjectStep> level2_obj_steps = {
        slaposSliceSupports
    };

    SLAPrintStep print_steps[] = { slapsMergeSlicesAndEval, slapsRasterize };
    
    double st = Steps::min_objstatus;

    BOOST_LOG_TRIVIAL(info) << "Start slicing process.";

#ifdef SLAPRINT_DO_BENCHMARK
    Benchmark bench;
#else
    struct {
        void start() {} void stop() {} double getElapsedSec() { return .0; }
    } bench;
#endif

    std::array<double, slaposCount + slapsCount> step_times {};

    auto apply_steps_on_objects =
        [this, &st, &printsteps, &step_times, &bench]
        (const std::vector<SLAPrintObjectStep> &steps)
    {
        double incr = 0;
        for (SLAPrintObject *po : m_objects) {
            for (SLAPrintObjectStep step : steps) {

                // Cancellation checking. Each step will check for
                // cancellation on its own and return earlier gracefully.
                // Just after it returns execution gets to this point and
                // throws the canceled signal.
                throw_if_canceled();

                st += incr;

                if (po->set_started(step)) {
                    m_report_status(*this, st, printsteps.label(step));
                    bench.start();
                    printsteps.execute(step, *po);
                    bench.stop();
                    step_times[step] += bench.getElapsedSec();
                    throw_if_canceled();
                    po->set_done(step);
                }
                
                incr = printsteps.progressrange(step);
            }
        }
    };

    apply_steps_on_objects(level1_obj_steps);
    apply_steps_on_objects(level2_obj_steps);

    st = Steps::max_objstatus;
    for(SLAPrintStep currentstep : print_steps) {
        throw_if_canceled();

        if (set_started(currentstep)) {
            m_report_status(*this, st, printsteps.label(currentstep));
            bench.start();
            printsteps.execute(currentstep);
            bench.stop();
            step_times[slaposCount + currentstep] += bench.getElapsedSec();
            throw_if_canceled();
            set_done(currentstep);
        }
        
        st += printsteps.progressrange(currentstep);
    }

    // If everything vent well
    m_report_status(*this, 100, _u8L("Slicing done"));

#ifdef SLAPRINT_DO_BENCHMARK
    std::string csvbenchstr;
    for (size_t i = 0; i < size_t(slaposCount); ++i)
        csvbenchstr += printsteps.label(SLAPrintObjectStep(i)) + ";";

    for (size_t i = 0; i < size_t(slapsCount); ++i)
        csvbenchstr += printsteps.label(SLAPrintStep(i)) + ";";

    csvbenchstr += "\n";
    for (double t : step_times) csvbenchstr += std::to_string(t) + ";";

    std::cout << "Performance stats: \n" << csvbenchstr << std::endl;
#endif

}

void SLAPrint::slice() {
    this->process();
    this->finalize();
    this->cleanup();
};

bool SLAPrint::invalidate_state_by_config_options(const std::vector<t_config_option_key> &opt_keys, bool &invalidate_all_model_objects)
{
    using namespace std::string_view_literals;

    if (opt_keys.empty())
        return false;

    static constexpr StaticSet steps_full = {
        "initial_layer_height"sv,
        "material_correction"sv,
        "material_correction_x"sv,
        "material_correction_y"sv,
        "material_correction_z"sv,
        "material_print_speed"sv,
        "relative_correction"sv,
        "relative_correction_x"sv,
        "relative_correction_y"sv,
        "relative_correction_z"sv,
        "absolute_correction"sv,
        "elefant_foot_compensation"sv,
        "elefant_foot_min_width"sv,
        "zcorrection_layers"sv,
        "gamma_correction"sv,
    };

    // Cache the plenty of parameters, which influence the final rasterization only,
    // or they are only notes not influencing the rasterization step.
    static constexpr StaticSet steps_rasterize = {
        "min_exposure_time"sv,
        "max_exposure_time"sv,
        "exposure_time"sv,
        "min_initial_exposure_time"sv,
        "max_initial_exposure_time"sv,
        "initial_exposure_time"sv,
        "display_width"sv,
        "display_height"sv,
        "display_pixels_x"sv,
        "display_pixels_y"sv,
        "display_mirror_x"sv,
        "display_mirror_y"sv,
        "display_orientation"sv,
        "sla_archive_format"sv,
        "sla_output_precision"sv,
        // tilt params
        "delay_before_exposure"sv,
        "delay_after_exposure"sv,
        "tower_hop_height"sv,
        "tower_speed"sv,
        "use_tilt"sv,
        "tilt_down_initial_speed"sv,
        "tilt_down_offset_steps"sv,
        "tilt_down_offset_delay"sv,
        "tilt_down_finish_speed"sv,
        "tilt_down_cycles"sv,
        "tilt_down_delay"sv,
        "tilt_up_initial_speed"sv,
        "tilt_up_offset_steps"sv,
        "tilt_up_offset_delay"sv,
        "tilt_up_finish_speed"sv,
        "tilt_up_cycles"sv,
        "tilt_up_delay"sv,
        "area_fill"sv,
    };

    static StaticSet steps_ignore = {
        "bed_shape"sv,
        "max_print_height"sv,
        "printer_technology"sv,
        "output_filename_format"sv,
        "fast_tilt_time"sv,
        "slow_tilt_time"sv,
        "high_viscosity_tilt_time"sv,
        "bottle_cost"sv,
        "bottle_volume"sv,
        "bottle_weight"sv,
        "material_density"sv,
        "material_ow_support_pillar_diameter"sv,
        "material_ow_support_head_front_diameter"sv,
        "material_ow_support_head_penetration"sv,
        "material_ow_support_head_width"sv,
        "material_ow_branchingsupport_pillar_diameter"sv,
        "material_ow_branchingsupport_head_front_diameter"sv,
        "material_ow_branchingsupport_head_penetration"sv,
        "material_ow_branchingsupport_head_width"sv,
        "material_ow_elefant_foot_compensation"sv,
        "material_ow_support_points_density_relative"sv,
        "material_ow_absolute_correction"sv,
        "printer_model"sv,
    };

    std::vector<SLAPrintStep> steps;
    std::vector<SLAPrintObjectStep> osteps;
    bool invalidated = false;

    for (std::string_view opt_key : opt_keys) {
        if (steps_rasterize.find(opt_key) != steps_rasterize.end()) {
            // These options only affect the final rasterization, or they are just notes without influence on the output,
            // so there is nothing to invalidate.
            steps.emplace_back(slapsMergeSlicesAndEval);
        } else if (steps_ignore.find(opt_key) != steps_ignore.end()) {
            // These steps have no influence on the output. Just ignore them.
        } else if (steps_full.find(opt_key) != steps_full.end()) {
            steps.emplace_back(slapsMergeSlicesAndEval);
            osteps.emplace_back(slaposObjectSlice);
            invalidate_all_model_objects = true;
        } else {
            // All values should be covered.
            assert(false);
        }
    }

    sort_remove_duplicates(steps);
    for (SLAPrintStep step : steps)
        invalidated |= this->invalidate_step(step);
    sort_remove_duplicates(osteps);
    for (SLAPrintObjectStep ostep : osteps)
        for (SLAPrintObject *object : m_objects)
            invalidated |= object->invalidate_step(ostep);
    return invalidated;
}

// Returns true if an object step is done on all objects and there's at least one object.
bool SLAPrint::is_step_done(SLAPrintObjectStep step) const
{
    if (m_objects.empty())
        return false;
    std::scoped_lock<std::mutex> lock(this->state_mutex());
    for (const SLAPrintObject *object : m_objects)
        if (! object->is_step_done_unguarded(step))
            return false;
    return true;
}

SLAPrintObject::SLAPrintObject(SLAPrint *print, ModelObject *model_object)
    : Inherited(print, model_object)
{}

bool SLAPrintObject::invalidate_step(SLAPrintObjectStep step)
{
    bool invalidated = Inherited::invalidate_step(step);
    // propagate to dependent steps
    if (step == slaposAssembly) {
        invalidated |= this->invalidate_all_steps();
    } else if (step == slaposHollowing) {
        invalidated |= this->invalidate_steps({ slaposDrillHoles, slaposObjectSlice, slaposSupportPoints, slaposSupportTree, slaposPad, slaposSliceSupports });
        invalidated |= m_print->invalidate_step(slapsMergeSlicesAndEval);
    } else if (step == slaposDrillHoles) {
        invalidated |= this->invalidate_steps({ slaposObjectSlice, slaposSupportPoints, slaposSupportTree, slaposPad, slaposSliceSupports });
        invalidated |= m_print->invalidate_step(slapsMergeSlicesAndEval);
    } else if (step == slaposObjectSlice) {
        invalidated |= this->invalidate_steps({ slaposSupportPoints, slaposSupportTree, slaposPad, slaposSliceSupports });
        invalidated |= m_print->invalidate_step(slapsMergeSlicesAndEval);
    } else if (step == slaposSupportPoints) {
        invalidated |= this->invalidate_steps({ slaposSupportTree, slaposPad, slaposSliceSupports });
        invalidated |= m_print->invalidate_step(slapsMergeSlicesAndEval);
    } else if (step == slaposSupportTree) {
        invalidated |= this->invalidate_steps({ slaposPad, slaposSliceSupports });
        invalidated |= m_print->invalidate_step(slapsMergeSlicesAndEval);
    } else if (step == slaposPad) {
        invalidated |= this->invalidate_steps({slaposSliceSupports});
        invalidated |= m_print->invalidate_step(slapsMergeSlicesAndEval);
    } else if (step == slaposSliceSupports) {
        invalidated |= m_print->invalidate_step(slapsMergeSlicesAndEval);
    }
    return invalidated;
}

bool SLAPrintObject::invalidate_all_steps()
{
    return Inherited::invalidate_all_steps() || m_print->invalidate_all_steps();
}

double SLAPrintObject::get_elevation() const {
    if (is_zero_elevation(m_config)) return 0.;

    bool en = m_config.supports_enable.getBool();

    double ret = en ? m_config.support_object_elevation.getFloat() : 0.;

    if(m_config.pad_enable.getBool()) {
        // Normally the elevation for the pad itself would be the thickness of
        // its walls but currently it is half of its thickness. Whatever it
        // will be in the future, we provide the config to the get_pad_elevation
        // method and we will have the correct value
        sla::PadConfig pcfg = make_pad_cfg(m_config);
        if(!pcfg.embed_object) ret += pcfg.required_elevation();
    }

    return ret;
}

double SLAPrintObject::get_current_elevation() const
{
    if (is_zero_elevation(m_config)) return 0.;

    bool has_supports = is_step_done(slaposSupportTree);
    bool has_pad      = is_step_done(slaposPad);

    if(!has_supports && !has_pad)
        return 0;
    else if(has_supports && !has_pad) {
        return m_config.support_object_elevation.getFloat();
    }

    return get_elevation();
}

Vec3d SLAPrint::relative_correction() const
{
    Vec3d corr(1., 1., 1.);

    if(printer_config().relative_correction.values.size() >= 2) {
        corr.x() = printer_config().relative_correction_x.value;
        corr.y() = printer_config().relative_correction_y.value;
        corr.z() = printer_config().relative_correction_z.value;
    }

    if(material_config().material_correction.values.size() >= 2) {
        corr.x() *= material_config().material_correction_x.value;
        corr.y() *= material_config().material_correction_y.value;
        corr.z() *= material_config().material_correction_z.value;
    }

    return corr;
}

using Domain::TriangleMesh;

namespace { // dummy empty static containers for return values in some methods
const std::vector<ExPolygons> EMPTY_SLICES;
const TriangleMesh EMPTY_MESH;
const indexed_triangle_set EMPTY_TRIANGLE_SET;
const ExPolygons EMPTY_SLICE;
const std::vector<Domain::SLA::SupportPoint> EMPTY_SUPPORT_POINTS;
}

const SliceRecord SliceRecord::EMPTY(0, std::nanf(""), 0.f);

const std::vector<ExPolygons> &SLAPrintObject::get_support_slices() const
{
    // assert(is_step_done(slaposSliceSupports));
    return (!m_support_slices.empty()) ? m_support_slices : EMPTY_SLICES;
}

const ExPolygons &SliceRecord::get_slice(SliceOrigin o) const
{
    size_t idx = o == soModel ? m_model_slices_idx : m_support_slices_idx;

    if(m_po == nullptr) return EMPTY_SLICE;

    const std::vector<ExPolygons>& v = o == soModel? m_po->get_model_slices() :
                                                     m_po->get_support_slices();

    return idx >= v.size() ? EMPTY_SLICE : v[idx];
}

const TriangleMesh& SLAPrintObject::support_mesh() const
{
    if (m_config.supports_enable.getBool() &&
        is_step_done(slaposSupportTree) &&
        m_preview.has_value() &&
        m_preview->support_structure)
        return *m_preview->support_structure;
    return EMPTY_MESH;
}

const TriangleMesh& SLAPrintObject::pad_mesh() const
{
    if (m_config.pad_enable.getBool() && is_step_done(slaposPad) && 
        m_preview.has_value() && m_preview->pad)
        return *m_preview->pad;
    return EMPTY_MESH;
}

std::shared_ptr<const Domain::TriangleMesh> SLAPrintObject::get_mesh_to_print() const
{
    if(!m_preview.has_value())
        return nullptr;
    return m_preview->mesh;
}

std::vector<csg::CSGPart> SLAPrintObject::get_parts_to_slice() const
{
    return get_parts_to_slice(slaposCount);
}

std::vector<csg::CSGPart>
SLAPrintObject::get_parts_to_slice(SLAPrintObjectStep untilstep) const
{
    auto laststep = last_completed_step();
    SLAPrintObjectStep s = std::min(untilstep, laststep);

    if (s == slaposCount)
        return {};

    std::vector<csg::CSGPart> ret;

    for (unsigned int step = 0; step <= s; ++step) {
        auto r = m_mesh_to_slice.equal_range(SLAPrintObjectStep(step));
        csg::copy_csgrange_shallow(Range{r.first, r.second}, std::back_inserter(ret));
    }

    return ret;
}

Domain::SLA::SupportPoints SLAPrintObject::transformed_support_points() const
{
    assert(model_object());
    auto spts = model_object()->sla_support_points;
    Transform3f tr = trafo().cast<float>();
    for (Domain::SLA::SupportPoint &suppt : spts) {
        suppt.pos = tr * suppt.pos;
    }
    return spts;
}

Domain::SLA::DrainHoles SLAPrintObject::transformed_drainhole_points() const
{
    assert(model_object());
    Domain::SLA::DrainHoles drainholes = drainholes; // Copy the drainholes
    sla::transform_drainhole_points(drainholes, trafo());
    return drainholes;
}

void SLAPrint::StatusReporter::operator()(SLAPrint &         p,
                                          double             st,
                                          const std::string &msg,
                                          unsigned           flags,
                                          const std::string &logmsg)
{
    m_st = st;
    BOOST_LOG_TRIVIAL(info)
        << st << "% " << msg << (logmsg.empty() ? "" : ": ") << logmsg
        << log_memory_info();

    p.set_status(int(std::round(st)), msg, flags);
}

namespace csg {

MeshBoolean::cgal::CGALMeshPtr get_cgalmesh(const CSGPartForStep &part)
{
    if (!part.cgalcache && csg::get_mesh(part)) {
        part.cgalcache = csg::get_cgalmesh(static_cast<const csg::CSGPart&>(part));
    }

    return part.cgalcache? clone(*part.cgalcache) : nullptr;
}

} // namespace csg

} // namespace Slic3r

// TODO: move function out of SLAPrint
// SLAResult result; // TODO: get it from SLAResultCache
using namespace Slic3r::Biz::Slicing;
void Slic3r::export_print(
    const std::string& fname,
    SLAResult& data,
    const ThumbnailsList& thumbnails,
    const std::string& projectname
)
{
    if (data.files.data.empty())
        throw ExportError(_u8L("No layer to export yet."));

    data.thumbnails = thumbnails;
    data.project_name = projectname; // ?? No idea why it is used

    // select format by
    switch (data.files.type) {
    case Sla::FileDataType::sl1_svg: [[fallthrough]];
    case Sla::FileDataType::sl1_png: store_sl1(fname, data); break;
    default:
        throw ExportError(_u8L("Unknown output file format"));
    }
}
