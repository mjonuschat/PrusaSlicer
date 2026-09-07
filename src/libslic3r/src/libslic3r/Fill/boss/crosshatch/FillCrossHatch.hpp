///|/ Copyright (c) BambuStudio 2023 Bambu Lab
///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_FillCrossHatch_hpp_
#define slic3r_FillCrossHatch_hpp_

#include "libslic3r/ExPolygon.hpp"
#include "libslic3r/Fill/FillBase.hpp"
#include "libslic3r/Polyline.hpp"
#include "libslic3r/libslic3r.h"

namespace Slic3r {

class FillCrossHatch : public Fill
{
public:
    Fill *clone() const override { return new FillCrossHatch(*this); };
    ~FillCrossHatch() override {}

    bool is_self_crossing() override { return false; }

protected:
    void _fill_surface_single(
        const FillParams              &params,
        unsigned int                   thickness_layers,
        const std::pair<float, Point> &direction,
        ExPolygon                      expolygon,
        Polylines                     &polylines_out) override;
};

} // namespace Slic3r

#endif // slic3r_FillCrossHatch_hpp_
