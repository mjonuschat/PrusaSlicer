///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "boss/features/jerk-scv-mcr/JerkScvMcrFeature.hpp"

#include "boss/foundation/ExtrusionContext.hpp"
#include "boss/foundation/MotionDynamics.hpp"
#include "libslic3r/ExtrudeConfig.hpp"
#include "libslic3r/ExtrusionRole.hpp"

namespace Slic3r::Boss {

namespace {

using Biz::Slicing::ExtrudeConfig;

// Mirrors the 2.9.x accel cascade's own role predicate, in the same priority
// order, since MCR selection rides on whichever role that cascade picked --
// each branch gated by that role's own acceleration value, not its MCR.
double resolve_minimum_cruise_ratio(const ExtrudeConfig& config, const ExtrusionContext& ctx)
{
    const std::size_t id = ctx.extruder_id;
    if (ctx.is_first_layer && config.first_layer_acceleration.at(id) > 0)
        return config.boss.first_layer_minimum_cruise_ratio.at(id);
    if (ctx.is_object_layer_over_raft && config.first_layer_acceleration_over_raft.at(id) > 0)
        return config.boss.first_layer_minimum_cruise_ratio_over_raft.at(id);
    if (ctx.role.is_bridge() && config.bridge_acceleration.at(id) > 0)
        return config.boss.bridge_minimum_cruise_ratio.at(id);
    if (ctx.role == ExtrusionRole::TopSolidInfill
        && config.top_solid_infill_acceleration.at(id) > 0)
        return config.boss.top_solid_infill_minimum_cruise_ratio.at(id);
    if (ctx.role.is_solid_infill() && config.solid_infill_acceleration.at(id) > 0)
        return config.boss.solid_infill_minimum_cruise_ratio.at(id);
    if (ctx.role.is_infill() && config.infill_acceleration.at(id) > 0)
        return config.boss.infill_minimum_cruise_ratio.at(id);
    if (ctx.role.is_external_perimeter() && config.external_perimeter_acceleration.at(id) > 0)
        return config.boss.external_perimeter_minimum_cruise_ratio.at(id);
    if (ctx.role.is_perimeter() && config.perimeter_acceleration.at(id) > 0)
        return config.boss.perimeter_minimum_cruise_ratio.at(id);
    return config.boss.default_minimum_cruise_ratio.at(id);
}

// Independent of resolve_minimum_cruise_ratio: gated per-branch on the role's own
// jerk value, under an outer default_jerk gate, so this can pick a different role
// than acceleration/MCR did.
int resolve_jerk(const ExtrudeConfig& config, const ExtrusionContext& ctx)
{
    const std::size_t id = ctx.extruder_id;
    if (config.boss.default_jerk.at(id) <= 0)
        return 0;
    if (ctx.is_first_layer && config.boss.first_layer_jerk.at(id) > 0)
        return config.boss.first_layer_jerk.at(id);
    if (ctx.is_object_layer_over_raft && config.boss.first_layer_jerk_over_raft.at(id) > 0)
        return config.boss.first_layer_jerk_over_raft.at(id);
    if (ctx.role.is_bridge() && config.boss.bridge_jerk.at(id) > 0)
        return config.boss.bridge_jerk.at(id);
    if (ctx.role == ExtrusionRole::TopSolidInfill && config.boss.top_solid_infill_jerk.at(id) > 0)
        return config.boss.top_solid_infill_jerk.at(id);
    if (ctx.role.is_solid_infill() && config.boss.solid_infill_jerk.at(id) > 0)
        return config.boss.solid_infill_jerk.at(id);
    if (ctx.role.is_infill() && config.boss.infill_jerk.at(id) > 0)
        return config.boss.infill_jerk.at(id);
    if (ctx.role.is_external_perimeter() && config.boss.external_perimeter_jerk.at(id) > 0)
        return config.boss.external_perimeter_jerk.at(id);
    if (ctx.role.is_perimeter() && config.boss.perimeter_jerk.at(id) > 0)
        return config.boss.perimeter_jerk.at(id);
    return config.boss.default_jerk.at(id);
}

// Travel's MCR is a direct per-role read, no acceleration-value gate: the 2.9.x
// source applies it unconditionally. Its jerk is gated by default_jerk > 0 &&
// travel_jerk > 0, using travel_jerk even for the short-distance role.
MotionDynamics resolve_travel(const ExtrudeConfig& config, const ExtrusionContext& ctx)
{
    const std::size_t id = ctx.extruder_id;
    const double mcr     = ctx.is_short_distance_travel ?
        config.boss.travel_short_distance_minimum_cruise_ratio.at(id) :
        config.boss.travel_minimum_cruise_ratio.at(id);
    const int role_jerk  = ctx.is_short_distance_travel ?
        config.boss.travel_short_distance_jerk.at(id) :
        config.boss.travel_jerk.at(id);
    const bool jerk_enabled =
        config.boss.default_jerk.at(id) > 0 && config.boss.travel_jerk.at(id) > 0;
    return MotionDynamics{0, mcr, static_cast<unsigned int>(jerk_enabled ? role_jerk : 0)};
}

} // namespace

std::optional<MotionDynamics> JerkScvMcrFeature::before_extrusion(const ExtrusionContext& ctx)
{
    if (ctx.extrude_config == nullptr)
        return std::nullopt;

    const ExtrudeConfig& config   = *ctx.extrude_config;
    const MotionDynamics dynamics = ctx.is_travel ?
        resolve_travel(config, ctx) :
        MotionDynamics{
            0,
            resolve_minimum_cruise_ratio(config, ctx),
            static_cast<unsigned int>(resolve_jerk(config, ctx))
        };

    if (dynamics.minimum_cruise_ratio <= 0.0 && dynamics.jerk == 0)
        return std::nullopt;
    return dynamics;
}

} // namespace Slic3r::Boss
