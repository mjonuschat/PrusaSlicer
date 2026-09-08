///|/ Copyright (c) 2026 Morton Jonuschat @mjonuschat
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "libslic3r/boss/perimeter/overlap/PerimeterOverlapValidation.hpp"

#include "libslic3r/HwConfigUtils.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/SlicingStatus.hpp"
#include "Slic3r/Domain/Percentage.hpp"

namespace Slic3r::Boss {

namespace {

double resolve_reference_width(const Domain::FloatOrPercentage &width, double nozzle_diameter)
{
    if (width.is_percentage())
        return nozzle_diameter * width.percentage().value / 100.0;
    if (width.float_value() > 0)
        return width.float_value();
    // Auto width (0): matches Flow::auto_extrusion_width's perimeter ratio.
    return nozzle_diameter * 1.125;
}

bool overlap_out_of_bounds(
    const Print &print, const std::string &key, double reference_width, double min_percent, double max_percent)
{
    if (reference_width <= 0.01)
        return false;
    const double min_mm = reference_width * min_percent / 100.0;
    const double max_mm = reference_width * max_percent / 100.0;
    for (const PrintObject *object : print.objects()) {
        for (const PrintRegion &region : object->all_regions()) {
            const Domain::FloatOrPercentage overlap = region.config().get<Domain::FloatOrPercentage>(key);
            if (overlap.is_percentage())
                continue;
            const double value = overlap.float_value();
            if (value < min_mm - 0.001 || value > max_mm + 0.001)
                return true;
        }
    }
    return false;
}

} // namespace

void validate_perimeter_overlap(const Print &print, std::vector<Biz::Slicing::Warning> &warnings)
{
    if (print.objects().empty())
        return;

    const double nozzle_diameter = Biz::Slicing::get_nozzle_diameter(print.config().hw_config(), 0);
    const PrintObject *first_object = print.objects().front();
    if (first_object->all_regions().empty())
        return;
    const PrintRegion &first_region = first_object->all_regions().front();
    const double perimeter_width = resolve_reference_width(
        first_region.config().get<Domain::FloatOrPercentage>("perimeter_extrusion_width"), nozzle_diameter);

    bool out_of_bounds = false;
    out_of_bounds |= overlap_out_of_bounds(print, "external_perimeter_overlap", perimeter_width, -100.0, 100.0);
    out_of_bounds |= overlap_out_of_bounds(print, "perimeter_perimeter_overlap", perimeter_width, -100.0, 80.0);

    if (out_of_bounds)
        warnings.push_back(Biz::Slicing::Warning{Biz::Slicing::WarningCode::PerimeterOverlapOutOfBounds});
}

} // namespace Slic3r::Boss
