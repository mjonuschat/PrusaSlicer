///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas
///|/ Copyright (c) SuperSlicer 2023 Remi Durand @supermerill
///|/ Copyright (c) 2023 Alexander Thor @Alexander-T-Moss
///|/ Copyright (c) 2024 - 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_GCode_SmallAreaInfillFlowCompensator_hpp_
#define slic3r_GCode_SmallAreaInfillFlowCompensator_hpp_

#include "../libslic3r.h"
#include "../PrintConfig.hpp"
#include "../ExtrusionRole.hpp"

#include <vector>

namespace Slic3r {

class SmallAreaInfillFlowCompensator
{
public:
    SmallAreaInfillFlowCompensator() = delete;
    explicit SmallAreaInfillFlowCompensator(const Slic3r::GCodeConfig &config);
    ~SmallAreaInfillFlowCompensator() = default;

    double modify_flow(const double line_length, const double dE, const ExtrusionRole role);

private:
    // Data points
    std::vector<double> eLengths;
    std::vector<double> flowComps;

    // Akima interpolation coefficients (per interval)
    std::vector<double> m_slopes;  // slopes at each knot
    std::vector<double> m_c;       // quadratic coefficients
    std::vector<double> m_d;       // cubic coefficients

    void compute_akima_coefficients();
    double flow_comp_model(const double line_length);

    double max_modified_length() {
        return eLengths.back();
    }
};

} // namespace Slic3r

#endif /* slic3r_GCode_SmallAreaInfillFlowCompensator_hpp_ */
