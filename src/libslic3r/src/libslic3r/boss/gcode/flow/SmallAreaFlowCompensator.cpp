///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena
///|/ Copyright (c) SuperSlicer 2023 Remi Durand @supermerill
///|/ Copyright (c) 2023 Alexander Thor @Alexander-T-Moss
///|/ Copyright (c) 2024 - 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "SmallAreaFlowCompensator.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

#include "Slic3r/Exception.hpp"
#include "boss/features/small-area-flow-compensation/SmallAreaFlowCompensationFeature.hpp"
#include "boss/foundation/ExtrusionContext.hpp"
#include "libslic3r/ExtrudeConfig.hpp"

namespace Slic3r::Boss {

SmallAreaFlowCompensator::SmallAreaFlowCompensator(std::vector<double> lengths, std::vector<double> factors)
{
    if (lengths.empty()) {
        throw Slic3r::InvalidArgument(
            "Small area infill flow compensation requires at least one length/factor pair");
    }
    if (lengths.size() != factors.size()) {
        throw Slic3r::InvalidArgument(
            "Small area infill flow compensation lengths and factors must have the same size");
    }

    if (lengths[0] != 0.0) {
        throw Slic3r::InvalidArgument(
            "First extrusion length for small area infill compensation length must be 0");
    }

    m_lengths.push_back(lengths[0]);
    m_factors.push_back(factors[0]);

    for (size_t i = 1; i < lengths.size(); ++i) {
        if (lengths[i] <= 0.0 || lengths[i] <= m_lengths.back()) {
            throw Slic3r::InvalidArgument("Extrusion lengths for subsequent points must be increasing");
        }
        if (factors[i] <= factors[i - 1]) {
            throw Slic3r::InvalidArgument(
                "Flow compensation factors must strictly increase with extrusion length");
        }
        m_lengths.push_back(lengths[i]);
        m_factors.push_back(factors[i]);
    }

    if (m_factors.back() != 1.0) {
        throw Slic3r::InvalidArgument(
            "Final compensation factor for small area infill flow compensation must be 1.0");
    }

    compute_akima_coefficients();
}

// Akima interpolation (1970) -- local C^1 interpolation using weighted slopes.
// Produces smoother curves than PCHIP at the steep-to-plateau transition
// without the overshoot of global cubic splines.
void SmallAreaFlowCompensator::compute_akima_coefficients()
{
    const size_t n = m_lengths.size();

    std::vector<double> delta(n - 1);
    for (size_t i = 0; i < n - 1; ++i)
        delta[i] = (m_factors[i + 1] - m_factors[i]) / (m_lengths[i + 1] - m_lengths[i]);

    // Extend delta with 2 ghost values at each end (Akima's boundary formula),
    // mirroring: delta[-1] = 2*delta[0] - delta[1], etc.
    auto d = [&](int i) -> double {
        if (i < 0) {
            int j = -i - 1;
            if (j >= (int) delta.size())
                j = (int) delta.size() - 1;
            return 2.0 * delta[0] - delta[std::min(j + 1, (int) delta.size() - 1)];
        }
        if (i >= (int) delta.size()) {
            int j    = i - (int) delta.size();
            int last = (int) delta.size() - 1;
            if (j >= (int) delta.size())
                j = last;
            return 2.0 * delta[last] - delta[std::max(last - j - 1, 0)];
        }
        return delta[i];
    };

    m_slopes.resize(n);
    for (size_t i = 0; i < n; ++i) {
        int    ii = (int) i;
        double w1 = std::abs(d(ii + 1) - d(ii));
        double w2 = std::abs(d(ii - 1) - d(ii - 2));

        if (w1 + w2 > 1e-30) {
            m_slopes[i] = (w1 * d(ii - 1) + w2 * d(ii)) / (w1 + w2);
        } else {
            // All four secants are nearly equal -- use simple average
            m_slopes[i] = 0.5 * (d(ii - 1) + d(ii));
        }
    }

    m_c.resize(n - 1);
    m_d.resize(n - 1);
    for (size_t i = 0; i < n - 1; ++i) {
        double h = m_lengths[i + 1] - m_lengths[i];
        m_c[i]   = (3.0 * delta[i] - 2.0 * m_slopes[i] - m_slopes[i + 1]) / h;
        m_d[i]   = (m_slopes[i] + m_slopes[i + 1] - 2.0 * delta[i]) / (h * h);
    }
}

double SmallAreaFlowCompensator::flow_comp_model(double line_length)
{
    if (line_length == 0 || line_length > max_modified_length())
        return 1.0;

    size_t lo = 0, hi = m_lengths.size() - 1;
    while (lo + 1 < hi) {
        size_t mid = (lo + hi) / 2;
        if (m_lengths[mid] <= line_length)
            lo = mid;
        else
            hi = mid;
    }

    // Evaluate the Hermite cubic polynomial on interval [lo, lo+1] using Horner's form
    double t      = line_length - m_lengths[lo];
    double result = m_factors[lo] + t * (m_slopes[lo] + t * (m_c[lo] + t * m_d[lo]));
    return std::clamp(result, 0.0, 1.0);
}

double SmallAreaFlowCompensator::modify_flow(double line_length, double dE, Slic3r::ExtrusionRole role)
{
    if (role == Slic3r::ExtrusionRole::SolidInfill || role == Slic3r::ExtrusionRole::TopSolidInfill)
        return dE * flow_comp_model(line_length);

    return dE;
}

namespace {

// Caches the compensator built from the last-seen ExtrudeConfig* so it is
// rebuilt only when the resolved config changes, not on every extrusion
// segment. thread_local because GCode export runs the extrusion pipeline
// across multiple TBB worker threads.
SmallAreaFlowCompensator &compensator_for_config(
    const Biz::Slicing::ExtrudeConfig *extrude_config, const BossExtrudeConfigOverrides &boss)
{
    static thread_local const Biz::Slicing::ExtrudeConfig *s_cached_config = nullptr;
    static thread_local std::unique_ptr<SmallAreaFlowCompensator> s_compensator;

    if (s_cached_config != extrude_config) {
        const std::vector<double> lengths = {
            boss.small_area_infill_flow_compensation_extrusion_length_0,
            boss.small_area_infill_flow_compensation_extrusion_length_1,
            boss.small_area_infill_flow_compensation_extrusion_length_2,
            boss.small_area_infill_flow_compensation_extrusion_length_3,
            boss.small_area_infill_flow_compensation_extrusion_length_4,
            boss.small_area_infill_flow_compensation_extrusion_length_5,
            boss.small_area_infill_flow_compensation_extrusion_length_6,
            boss.small_area_infill_flow_compensation_extrusion_length_7,
            boss.small_area_infill_flow_compensation_extrusion_length_8,
            boss.small_area_infill_flow_compensation_extrusion_length_9,
        };
        const std::vector<double> factors = {
            boss.small_area_infill_flow_compensation_compensation_factor_0,
            boss.small_area_infill_flow_compensation_compensation_factor_1,
            boss.small_area_infill_flow_compensation_compensation_factor_2,
            boss.small_area_infill_flow_compensation_compensation_factor_3,
            boss.small_area_infill_flow_compensation_compensation_factor_4,
            boss.small_area_infill_flow_compensation_compensation_factor_5,
            boss.small_area_infill_flow_compensation_compensation_factor_6,
            boss.small_area_infill_flow_compensation_compensation_factor_7,
            boss.small_area_infill_flow_compensation_compensation_factor_8,
            boss.small_area_infill_flow_compensation_compensation_factor_9,
        };

        s_compensator   = std::make_unique<SmallAreaFlowCompensator>(lengths, factors);
        s_cached_config = extrude_config;
    }

    return *s_compensator;
}

} // namespace

double SmallAreaFlowCompensationFeature::modify_flow(double dE, const ExtrusionContext &ctx)
{
    if (ctx.role != Slic3r::ExtrusionRole::SolidInfill && ctx.role != Slic3r::ExtrusionRole::TopSolidInfill)
        return dE;

    if (ctx.extrude_config == nullptr)
        return dE;

    const Slic3r::Boss::BossExtrudeConfigOverrides &boss = ctx.extrude_config->boss;
    if (!boss.small_area_infill_flow_compensation)
        return dE;

    return compensator_for_config(ctx.extrude_config, boss).modify_flow(ctx.path_length, dE, ctx.role);
}

} // namespace Slic3r::Boss
