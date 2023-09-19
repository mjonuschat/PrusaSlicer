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
#include <boost/log/trivial.hpp>
// #include "../LocalesUtils.hpp"
// #include "../GCode.hpp"

#include "SmallAreaInfillFlowCompensator.hpp"
// #include "fast_float/fast_float.h"
// #include "GCodeWriter.hpp"

namespace Slic3r {

// Default flow drop off value, used when the input is invalid.
static constexpr int DEFAULT_FLOW_DROP_OFF = 12;

SmallAreaInfillFlowCompensator::SmallAreaInfillFlowCompensator(const Slic3r::GCodeConfig &config)
    : maxModifiedLength(config.small_area_infill_flow_compensation_max_length.value)
    , minFlowPercent(config.small_area_infill_flow_compensation_minimum_flow.get_abs_value(1.0))
    , flowDropOff(config.small_area_infill_flow_compensation_flow_dropoff.value)
{
    if (flowDropOff % 2 != 0) {
        BOOST_LOG_TRIVIAL(warning) << "Flow drop off needs to be a multiple of 2, using default value";
        flowDropOff = DEFAULT_FLOW_DROP_OFF;
    }
}

double SmallAreaInfillFlowCompensator::flow_comp_model(const double line_length)
{
    if (line_length == 0 || line_length > maxModifiedLength) {
        return 1.0;
    }

    double magicNumber = (minFlowPercent - 1) * pow(maxModifiedLength, -1 * flowDropOff);
    return magicNumber * pow(line_length - maxModifiedLength, flowDropOff) + 1;
}

double SmallAreaInfillFlowCompensator::modify_flow(const double line_length, const double dE, const ExtrusionRole role)
{
    if (role == ExtrusionRole::SolidInfill || role == ExtrusionRole::TopSolidInfill) {
        return dE * flow_comp_model(line_length);
    }

    return dE;
}

} // namespace Slic3r
