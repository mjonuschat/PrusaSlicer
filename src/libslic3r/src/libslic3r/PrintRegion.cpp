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
    return extruder;
}

Flow PrintRegion::flow(const PrintObject &object, FlowRole role, double layer_height, bool first_layer) const
{
    const PrintConfigView &print_config = object.print()->config();
    Domain::FloatOrPercentage config_width;
    // Get extrusion width from configuration.
    // (might be an absolute value, or a percent value, or zero for auto)
    if (first_layer && !print_config.get<Domain::FloatOrPercentage>("first_layer_extrusion_width").is_zero()) {
        config_width = print_config.get<Domain::FloatOrPercentage>("first_layer_extrusion_width");
    } else if (role == frExternalPerimeter) {
        config_width = m_config.get<Domain::FloatOrPercentage>("external_perimeter_extrusion_width");
    } else if (role == frPerimeter) {
        config_width = m_config.get<Domain::FloatOrPercentage>("perimeter_extrusion_width");
    } else if (role == frInfill) {
        config_width = m_config.get<Domain::FloatOrPercentage>("infill_extrusion_width");
    } else if (role == frSolidInfill) {
        config_width = m_config.get<Domain::FloatOrPercentage>("solid_infill_extrusion_width");
    } else if (role == frTopSolidInfill) {
        config_width = m_config.get<Domain::FloatOrPercentage>("top_infill_extrusion_width");
    } else {
        throw Slic3r::InvalidArgument("Unknown role");
    }

    if (config_width.is_zero())
        config_width = object.config().get<Domain::FloatOrPercentage>("extrusion_width");
    
    // Get the configured nozzle_diameter for the extruder associated to the flow role requested.
    // Here this->extruder(role) - 1 may underflow to MAX_INT, but then the get_at() will follback to zero'th element, so everything is all right.
    auto nozzle_diameter = float(print_config.get<std::vector<double>>("nozzle_diameter").at(this->extruder(role) - 1));
    return Flow::new_from_config_width(role, config_width, nozzle_diameter, float(layer_height));
}

double PrintRegion::nozzle_dmr_avg(const PrintConfigView &print_config) const
{
    return (print_config.get<std::vector<double>>("nozzle_diameter").at(m_config.get<int>("perimeter_extruder")    - 1) + 
            print_config.get<std::vector<double>>("nozzle_diameter").at(m_config.get<int>("infill_extruder")       - 1) + 
            print_config.get<std::vector<double>>("nozzle_diameter").at(m_config.get<int>("solid_infill_extruder") - 1)) / 3.;
}

double PrintRegion::bridging_height_avg(const PrintConfigView &print_config) const
{
    return this->nozzle_dmr_avg(print_config) * sqrt(m_config.get<double>("bridge_flow_ratio"));
}

void PrintRegion::collect_object_printing_extruders(const Domain::ConfigView& config, const bool has_brim, std::vector<unsigned int> &object_extruders)
{
    // These checks reflect the same logic used in the GUI for enabling/disabling extruder selection fields.
    auto num_extruders = (int)config.get<std::vector<double>>("nozzle_diameter").size();
    auto emplace_extruder = [num_extruders, &object_extruders](int extruder_id) {
    	int i = std::max(0, extruder_id - 1);
        object_extruders.emplace_back((i >= num_extruders) ? 0 : i);
    };
    if (config.get<int>("perimeters") > 0 || has_brim)
    	emplace_extruder(config.get<int>("perimeter_extruder"));
    if (config.get<Domain::Percentage>("fill_density") > Domain::Percentage{0})
    	emplace_extruder(config.get<int>("infill_extruder"));
    if (config.get<int>("top_solid_layers") > 0 || config.get<int>("bottom_solid_layers") > 0)
    	emplace_extruder(config.get<int>("solid_infill_extruder"));
}

void PrintRegion::collect_object_printing_extruders(const Print &print, std::vector<unsigned int> &object_extruders) const
{
    // PrintRegion, if used by some PrintObject, shall have all the extruders set to an existing printer extruder.
    // If not, then there must be something wrong with the Print::apply() function.
#ifndef NDEBUG
    auto num_extruders = int(print.config().get<std::vector<double>>("nozzle_diameter").size());
    assert(this->config().get<int>("perimeter_extruder")    <= num_extruders);
    assert(this->config().get<int>("infill_extruder")       <= num_extruders);
    assert(this->config().get<int>("solid_infill_extruder") <= num_extruders);
#endif
    collect_object_printing_extruders(this->config(), print.has_brim(), object_extruders);
}

}
