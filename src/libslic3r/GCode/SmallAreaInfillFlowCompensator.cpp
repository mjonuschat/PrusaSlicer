///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, Oleksandra Iushchenko @YuSanka, Lukáš Matěna
///@lukasmatena
///|/ Copyright (c) SuperSlicer 2023 Remi Durand @supermerill
///|/ Copyright (c) 2023 Alexander Thor @Alexander-T-Moss
///|/ Copyright (c) 2024 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <math.h>
#include <cstring>
#include <cfloat>
#include <array>

#include "../libslic3r.h"
#include "../PrintConfig.hpp"
#include "../Utils.hpp"

#include "SmallAreaInfillFlowCompensator.hpp"

namespace Slic3r {

SmallAreaInfillFlowCompensator::SmallAreaInfillFlowCompensator(const Slic3r::GCodeConfig &config)
{
    const std::array<double, 10> lengths = {
        config.small_area_infill_flow_compensation_extrusion_length_0,
        config.small_area_infill_flow_compensation_extrusion_length_1,
        config.small_area_infill_flow_compensation_extrusion_length_2,
        config.small_area_infill_flow_compensation_extrusion_length_3,
        config.small_area_infill_flow_compensation_extrusion_length_4,
        config.small_area_infill_flow_compensation_extrusion_length_5,
        config.small_area_infill_flow_compensation_extrusion_length_6,
        config.small_area_infill_flow_compensation_extrusion_length_7,
        config.small_area_infill_flow_compensation_extrusion_length_8,
        config.small_area_infill_flow_compensation_extrusion_length_9,
    };

    const std::array<double, 10> factors = {
        config.small_area_infill_flow_compensation_compensation_factor_0,
        config.small_area_infill_flow_compensation_compensation_factor_1,
        config.small_area_infill_flow_compensation_compensation_factor_2,
        config.small_area_infill_flow_compensation_compensation_factor_3,
        config.small_area_infill_flow_compensation_compensation_factor_4,
        config.small_area_infill_flow_compensation_compensation_factor_5,
        config.small_area_infill_flow_compensation_compensation_factor_6,
        config.small_area_infill_flow_compensation_compensation_factor_7,
        config.small_area_infill_flow_compensation_compensation_factor_8,
        config.small_area_infill_flow_compensation_compensation_factor_9,
    };

    if (!Slic3r::nearly_equal(lengths[0], 0.0)) {
        throw Slic3r::InvalidArgument("First extrusion length for small area infill compensation length must be 0");
    }

    eLengths.push_back(lengths[0]);
    flowComps.push_back(factors[0]);

    for (size_t i = 1; i < lengths.size(); ++i) {
        if (lengths[i] <= 0.0 || lengths[i] <= eLengths.back()) {
            throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
        }
        eLengths.push_back(lengths[i]);
        flowComps.push_back(factors[i]);
    }

    if (!nearly_equal(flowComps.back(), 1.0)) {
        throw Slic3r::InvalidArgument("Final compensation factor for small area infill flow compensation must be 1.0");
    }

    flowModel.set_points(eLengths, flowComps);
}

double SmallAreaInfillFlowCompensator::flow_comp_model(const double line_length)
{
    if (line_length == 0 || line_length > max_modified_length()) {
        return 1.0;
    }

    return flowModel(line_length);
}

double SmallAreaInfillFlowCompensator::modify_flow(const double line_length, const double dE, const ExtrusionRole role)
{
    if (role == ExtrusionRole::SolidInfill || role == ExtrusionRole::TopSolidInfill) {
        return dE * flow_comp_model(line_length);
    }

    return dE;
}

} // namespace Slic3r
