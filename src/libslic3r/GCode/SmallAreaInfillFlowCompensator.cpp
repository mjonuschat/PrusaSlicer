///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, Oleksandra Iushchenko @YuSanka, Lukáš Matěna
///@lukasmatena
///|/ Copyright (c) SuperSlicer 2023 Remi Durand @supermerill
///|/ Copyright (c) 2023 Alexander Thor @Alexander-T-Moss
///|/ Copyright (c) 2024 - 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include <cmath>
#include <cstring>
#include <cfloat>
#include <array>
#include <algorithm>

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
        if (factors[i] <= factors[i - 1]) {
            throw Slic3r::InvalidArgument("Flow compensation factors must strictly increase with extrusion length");
        }
        eLengths.push_back(lengths[i]);
        flowComps.push_back(factors[i]);
    }

    if (!nearly_equal(flowComps.back(), 1.0)) {
        throw Slic3r::InvalidArgument("Final compensation factor for small area infill flow compensation must be 1.0");
    }

    compute_akima_coefficients();
}

// Akima interpolation (1970) — local C^1 interpolation using weighted slopes.
// Produces smoother curves than PCHIP at the steep-to-plateau transition
// without the overshoot of global cubic splines.
void SmallAreaInfillFlowCompensator::compute_akima_coefficients()
{
    const size_t n = eLengths.size();

    // Compute secant slopes between consecutive points
    std::vector<double> delta(n - 1);
    for (size_t i = 0; i < n - 1; ++i)
        delta[i] = (flowComps[i + 1] - flowComps[i]) / (eLengths[i + 1] - eLengths[i]);

    // Extend delta with 2 ghost values at each end (Akima's boundary formula)
    // Use mirroring: delta[-1] = 2*delta[0] - delta[1], etc.
    auto d = [&](int i) -> double {
        if (i < 0) {
            // Mirror below: delta[-1] = 2*delta[0] - delta[1], delta[-2] = 2*delta[0] - delta[2]
            int j = -i - 1;
            if (j >= (int)delta.size()) j = (int)delta.size() - 1;
            return 2.0 * delta[0] - delta[std::min(j + 1, (int)delta.size() - 1)];
        }
        if (i >= (int)delta.size()) {
            // Mirror above
            int j = i - (int)delta.size();
            int last = (int)delta.size() - 1;
            if (j >= (int)delta.size()) j = last;
            return 2.0 * delta[last] - delta[std::max(last - j - 1, 0)];
        }
        return delta[i];
    };

    // Compute slopes at each knot using Akima's weighted formula
    m_slopes.resize(n);
    for (size_t i = 0; i < n; ++i) {
        int ii = (int)i;
        double w1 = std::abs(d(ii + 1) - d(ii));       // |delta[i+1] - delta[i]|
        double w2 = std::abs(d(ii - 1) - d(ii - 2));   // |delta[i-1] - delta[i-2]|

        if (w1 + w2 > 1e-30) {
            // Weighted average of adjacent slopes
            m_slopes[i] = (w1 * d(ii - 1) + w2 * d(ii)) / (w1 + w2);
        } else {
            // All four secants are nearly equal — use simple average
            m_slopes[i] = 0.5 * (d(ii - 1) + d(ii));
        }
    }

    // Compute Hermite cubic coefficients for each interval
    m_c.resize(n - 1);
    m_d.resize(n - 1);
    for (size_t i = 0; i < n - 1; ++i) {
        double h = eLengths[i + 1] - eLengths[i];
        m_c[i] = (3.0 * delta[i] - 2.0 * m_slopes[i] - m_slopes[i + 1]) / h;
        m_d[i] = (m_slopes[i] + m_slopes[i + 1] - 2.0 * delta[i]) / (h * h);
    }
}

double SmallAreaInfillFlowCompensator::flow_comp_model(const double line_length)
{
    if (line_length == 0 || line_length > max_modified_length()) {
        return 1.0;
    }

    // Binary search for the interval containing line_length
    size_t lo = 0, hi = eLengths.size() - 1;
    while (lo + 1 < hi) {
        size_t mid = (lo + hi) / 2;
        if (eLengths[mid] <= line_length)
            lo = mid;
        else
            hi = mid;
    }

    // Evaluate the Hermite cubic polynomial on interval [lo, lo+1] using Horner's form
    double t = line_length - eLengths[lo];
    double result = flowComps[lo] + t * (m_slopes[lo] + t * (m_c[lo] + t * m_d[lo]));
    return std::clamp(result, 0.0, 1.0);
}

double SmallAreaInfillFlowCompensator::modify_flow(const double line_length, const double dE, const ExtrusionRole role)
{
    if (role == ExtrusionRole::SolidInfill || role == ExtrusionRole::TopSolidInfill) {
        return dE * flow_comp_model(line_length);
    }

    return dE;
}

} // namespace Slic3r
