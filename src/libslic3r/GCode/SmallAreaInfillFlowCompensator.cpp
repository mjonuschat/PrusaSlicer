///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, Oleksandra Iushchenko @YuSanka, Lukáš Matěna
///@lukasmatena
///|/ Copyright (c) SuperSlicer 2023 Remi Durand @supermerill
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <math.h>
// #include <memory.h>
#include <cstring>
#include <cfloat>

#include "../libslic3r.h"
#include "../PrintConfig.hpp"
// #include "../LocalesUtils.hpp"
// #include "../GCode.hpp"

#include "SmallAreaInfillFlowCompensator.hpp"
// #include "fast_float/fast_float.h"
// #include "GCodeWriter.hpp"

namespace Slic3r {

bool nearly_equal(double a, double b)
{
  return std::nextafter(a, std::numeric_limits<double>::lowest()) <= b
    && std::nextafter(a, std::numeric_limits<double>::max()) >= b;
}

SmallAreaInfillFlowCompensator::SmallAreaInfillFlowCompensator(const Slic3r::GCodeConfig &config)
{
    auto curLength = 0.0;

    if(nearly_equal(config.small_area_infill_flow_compensation_extrusion_length_0, 0.0)) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_0);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_0);
    } else {
        throw Slic3r::InvalidArgument("First extrusion length for small area infill compensation length must be 0");
    }

    curLength = config.small_area_infill_flow_compensation_extrusion_length_1;
    if(curLength > 0.0 and curLength > eLengths.back()) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_1);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_1);
    } else {
        throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
    }

    curLength = config.small_area_infill_flow_compensation_extrusion_length_2;
    if(curLength > 0.0 and curLength > eLengths.back()) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_2);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_2);
    } else {
        throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
    }

    curLength = config.small_area_infill_flow_compensation_extrusion_length_3;
    if(curLength > 0.0 and curLength > eLengths.back()) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_3);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_3);
    } else {
        throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
    }

    curLength = config.small_area_infill_flow_compensation_extrusion_length_4;
    if(curLength > 0.0 and curLength > eLengths.back()) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_4);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_4);
    } else {
        throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
    }

    curLength = config.small_area_infill_flow_compensation_extrusion_length_5;
    if(curLength > 0.0 and curLength > eLengths.back()) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_5);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_5);
    } else {
        throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
    }

    curLength = config.small_area_infill_flow_compensation_extrusion_length_6;
    if(curLength > 0.0 and curLength > eLengths.back()) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_6);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_6);
    } else {
        throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
    }

    curLength = config.small_area_infill_flow_compensation_extrusion_length_7;
    if(curLength > 0.0 and curLength > eLengths.back()) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_7);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_7);
    } else {
        throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
    }

    curLength = config.small_area_infill_flow_compensation_extrusion_length_8;
    if(curLength > 0.0 and curLength > eLengths.back()) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_8);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_8);
    } else {
        throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
    }

    curLength = config.small_area_infill_flow_compensation_extrusion_length_9;
    if(curLength > 0.0 and curLength > eLengths.back()) {
        eLengths.push_back(config.small_area_infill_flow_compensation_extrusion_length_9);
        flowComps.push_back(config.small_area_infill_flow_compensation_compensation_factor_9);
    } else {
        throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
    }

    if(!nearly_equal(flowComps.back(), 1.0)) {
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
