///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas
///|/ Copyright (c) SuperSlicer 2023 Remi Durand @supermerill
///|/ Copyright (c) 2023 Alexander Thor @Alexander-T-Moss
///|/ Copyright (c) 2024 - 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <vector>

#include "libslic3r/ExtrusionRole.hpp"

namespace Slic3r::Boss {

class SmallAreaFlowCompensator
{
public:
    SmallAreaFlowCompensator() = delete;
    SmallAreaFlowCompensator(std::vector<double> lengths, std::vector<double> factors);
    ~SmallAreaFlowCompensator() = default;

    double modify_flow(double line_length, double dE, Slic3r::ExtrusionRole role);

private:
    std::vector<double> m_lengths;
    std::vector<double> m_factors;

    std::vector<double> m_slopes;
    std::vector<double> m_c;
    std::vector<double> m_d;

    void compute_akima_coefficients();
    double flow_comp_model(double line_length);

    double max_modified_length() const
    {
        return m_lengths.back();
    }
};

} // namespace Slic3r::Boss
