///|/ Copyright (c) Prusa Research 2023 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef GRAVITYKERNEL_HPP
#define GRAVITYKERNEL_HPP

#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Biz/Algorithms/Scaling.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <arrange/NFP/NFPArrangeItemTraits.hpp>
#include <arrange/Beds.hpp>

#include "KernelUtils.hpp"

namespace Slic3r::arr2 {

struct GravityKernel {
    std::optional<Domain::Vec2crd> sink;
    std::optional<Domain::Vec2crd> item_sink;
    Domain::Vec2d active_sink;

    GravityKernel(Domain::Vec2crd gravity_center) :
        sink{gravity_center}, active_sink{Biz::Algorithms::Scaling::unscaled<double>(gravity_center)} {}

    GravityKernel() = default;

    template<class ArrItem>
    double placement_fitness(const ArrItem &itm, const Domain::Vec2crd &transl) const
    {
        Domain::Vec2d center = Biz::Algorithms::Scaling::unscaled<double>(envelope_centroid(itm));

        center += Biz::Algorithms::Scaling::unscaled<double>(transl);

        return - (center - active_sink).squaredNorm();
    }

    template<class ArrItem, class Bed, class Ctx, class RemIt>
    bool on_start_packing(ArrItem &itm,
                          const Bed &bed,
                          const Ctx &packing_context,
                          const Range<RemIt> & /*remaining_items*/)
    {
        bool ret = false;

        item_sink = get_gravity_sink(itm);

        if (!sink) {
            sink = Biz::Algorithms::BoundingBox::center(bounding_box(bed));
        }

        if (item_sink)
            active_sink = Biz::Algorithms::Scaling::unscaled<double>(*item_sink);
        else
            active_sink = Biz::Algorithms::Scaling::unscaled<double>(*sink);

        ret = find_initial_position(itm, Biz::Algorithms::Scaling::scaled(active_sink), bed, packing_context);

        return ret;
    }

    template<class ArrItem> bool on_item_packed(ArrItem &itm) { return true; }
};

} // namespace Slic3r::arr2

#endif // GRAVITYKERNEL_HPP
