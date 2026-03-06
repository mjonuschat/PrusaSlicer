///|/ Copyright (c) Prusa Research 2017 - 2021 Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena
///|/ Copyright (c) Slic3r 2014 - 2015 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2015 Maksim Derbasov @ntfshard
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstddef>

#include "Slic3r/Exception.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Flow.hpp"
#include "libslic3r/libslic3r.h"
#include "libslic3r/HwConfigUtils.hpp"

namespace Slic3r {

// 1-based extruder identifier for this region and role.
unsigned int PrintRegion::extruder(FlowRole role) const
{
    size_t extruder = 0;
    if (role == frPerimeter || role == frExternalPerimeter)
        extruder = m_config.get<int>("perimeter_extruder");
    else if (role == frInfill)
        extruder = m_config.get<int>("infill_extruder");
    else if (role == frSolidInfill || role == frTopSolidInfill)
        extruder = m_config.get<int>("solid_infill_extruder");
    else
        throw Slic3r::InvalidArgument("Unknown role");

    if (extruder == 0) {
        return 1;
    }

    return extruder;
}

Flow PrintRegion::flow(const PrintObject &object, FlowRole role, double layer_height, bool first_layer) const
{
    const unsigned extruder_id{extruder(role) - 1};

    const PrintConfigView &print_config = object.print()->config();
    Domain::FloatOrPercentage config_width;
    // Get extrusion width from configuration.
    // (might be an absolute value, or a percent value, or zero for auto)
    if (first_layer && !print_config.get<std::vector<Domain::FloatOrPercentage>>("first_layer_extrusion_width").at(extruder_id).is_zero()) {
        config_width = print_config.get<std::vector<Domain::FloatOrPercentage>>("first_layer_extrusion_width").at(extruder_id);
    } else if (role == frExternalPerimeter) {
        config_width = m_config.get<std::vector<Domain::FloatOrPercentage>>("external_perimeter_extrusion_width").at(extruder_id);
    } else if (role == frPerimeter) {
        config_width = m_config.get<std::vector<Domain::FloatOrPercentage>>("perimeter_extrusion_width").at(extruder_id);
    } else if (role == frInfill) {
        config_width = m_config.get<std::vector<Domain::FloatOrPercentage>>("infill_extrusion_width").at(extruder_id);
    } else if (role == frSolidInfill) {
        config_width = m_config.get<std::vector<Domain::FloatOrPercentage>>("solid_infill_extrusion_width").at(extruder_id);
    } else if (role == frTopSolidInfill) {
        config_width = m_config.get<std::vector<Domain::FloatOrPercentage>>("top_infill_extrusion_width").at(extruder_id);
    } else {
        throw Slic3r::InvalidArgument("Unknown role");
    }

    if (config_width.is_zero())
        config_width = object.config().get<std::vector<Domain::FloatOrPercentage>>("extrusion_width").at(extruder_id);

    // Get the configured nozzle_diameter for the extruder associated to the flow role requested.
    // Here this->extruder(role) is > 0.
    auto nozzle_diameter = float(Biz::Slicing::get_nozzle_diameter(print_config.hw_config(), extruder_id));
    return Flow::new_from_config_width(role, config_width, nozzle_diameter, float(layer_height));
}

double PrintRegion::nozzle_dmr_avg(const PrintConfigView& print_config) const
{
    using Biz::Slicing::get_nozzle_diameter;
    return (get_nozzle_diameter(
                print_config.hw_config(),
                m_config.get<int>("perimeter_extruder") - 1
            )
            + get_nozzle_diameter(
                print_config.hw_config(),
                m_config.get<int>("infill_extruder") - 1
            )
            + get_nozzle_diameter(
                print_config.hw_config(),
                m_config.get<int>("solid_infill_extruder") - 1
            ))
        / 3.;
}

double PrintRegion::bridging_height_avg(const PrintConfigView &print_config) const
{
    const double bridge_flow_avg{(
        extruder_config_value<double>("bridge_flow_ratio", FlowRole::frPerimeter) +
        extruder_config_value<double>("bridge_flow_ratio", FlowRole::frSolidInfill)
    ) / 2.0};
    return this->nozzle_dmr_avg(print_config) * bridge_flow_avg;
}

void PrintRegion::collect_object_printing_extruders(
    const PrintRegionConfigView& config,
    const bool has_brim,
    std::vector<unsigned int>& object_extruders
)
{
    // These checks reflect the same logic used in the GUI for enabling/disabling extruder selection fields.
    auto num_extruders    = (int) config.hw_config().material_slot_count();
    auto emplace_extruder = [num_extruders, &object_extruders](int extruder_id)
    {
        int i = std::max(0, extruder_id - 1);
        object_extruders.emplace_back((i >= num_extruders) ? 0 : i);
    };
    const int perimeter_extruder{config.get<int>("perimeter_extruder")};
    ASSERT(perimeter_extruder > 0);
    if (config.get<std::vector<int>>("perimeters").at(perimeter_extruder - 1) > 0 || has_brim)
        emplace_extruder(perimeter_extruder);

    const int infill_extruder{config.get<int>("infill_extruder")};
    ASSERT(infill_extruder > 0);
    if (config.get<std::vector<Domain::Percentage>>("fill_density").at(infill_extruder - 1) > Domain::Percentage{0})
        emplace_extruder(infill_extruder);

    const int solid_infill_extruder{config.get<int>("solid_infill_extruder")};
    if (config.get<std::vector<int>>("top_solid_layers").at(solid_infill_extruder - 1) > 0
        || config.get<std::vector<int>>("bottom_solid_layers").at(solid_infill_extruder - 1) > 0)
        emplace_extruder(solid_infill_extruder);
}

double PrintRegion::nozzle_diameter(FlowRole role) const
{
    return Biz::Slicing::get_nozzle_diameter(m_config.hw_config(), extruder(role) - 1);
}

void PrintRegion::collect_object_printing_extruders(const Print &print, std::vector<unsigned int> &object_extruders) const
{
    // PrintRegion, if used by some PrintObject, shall have all the extruders set to an existing printer extruder.
    // If not, then there must be something wrong with the Print::apply() function.
    auto num_extruders = int(print.config().hw_config().material_slot_count());
    ASSERT(this->config().get<int>("perimeter_extruder") <= num_extruders);
    ASSERT(this->config().get<int>("infill_extruder") <= num_extruders);
    ASSERT(this->config().get<int>("solid_infill_extruder") <= num_extruders);
    collect_object_printing_extruders(this->config(), print.has_brim(), object_extruders);
}

}
