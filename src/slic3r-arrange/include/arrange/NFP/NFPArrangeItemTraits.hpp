///|/ Copyright (c) Prusa Research 2023 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef NFPARRANGEITEMTRAITS_HPP
#define NFPARRANGEITEMTRAITS_HPP

#include <numeric>
#include <type_traits>

#include <arrange/ArrangeBase.hpp>
#include "Slic3r/Biz/Algorithms/BoundingBox.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/ExPolygon.hpp"
#include "Slic3r/Domain/Polygon.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::arr2 {

// Additional methods that an ArrangeItem object has to implement in order
// to be usable with PackStrategyNFP.
template<class ArrItem, class En = void> struct NFPArrangeItemTraits_
{
    template<class Context, class Bed, class StopCond = DefaultStopCondition>
    static Domain::ExPolygons calculate_nfp(const ArrItem &item,
                                            const Context &packing_context,
                                            const Bed &bed,
                                            StopCond stop_condition = {})
    {
        static_assert(always_false<ArrItem>::value,
                      "NFP unimplemented for this item type.");
        return {};
    }

    static Domain::Vec2crd reference_vertex(const ArrItem& item)
    {
        return item.reference_vertex();
    }

    static Domain::BoundingBox2crd envelope_bounding_box(const ArrItem& itm)
    {
        return itm.envelope_bounding_box();
    }

    static Domain::BoundingBox2crd fixed_bounding_box(const ArrItem& itm)
    {
        return itm.fixed_bounding_box();
    }

    static const Domain::Polygons& envelope_outline(const ArrItem& itm)
    {
        return itm.envelope_outline();
    }

    static const Domain::Polygons& fixed_outline(const ArrItem& itm)
    {
        return itm.fixed_outline();
    }

    static const Domain::Polygon& envelope_convex_hull(const ArrItem& itm)
    {
        return itm.envelope_convex_hull();
    }

    static const Domain::Polygon& fixed_convex_hull(const ArrItem& itm)
    {
        return itm.fixed_convex_hull();
    }

    static double envelope_area(const ArrItem& itm)
    {
        return itm.envelope_area();
    }

    static double fixed_area(const ArrItem& itm)
    {
        return itm.fixed_area();
    }

    static auto allowed_rotations(const ArrItem&)
    {
        return std::array{0.};
    }

    static Domain::Vec2crd fixed_centroid(const ArrItem& itm)
    {
        return fixed_bounding_box(itm).center();
    }

    static Domain::Vec2crd envelope_centroid(const ArrItem& itm)
    {
        return Biz::Algorithms::BoundingBox::center(envelope_bounding_box(itm));
    }
};

template<class T>
using NFPArrangeItemTraits = NFPArrangeItemTraits_<StripCVRef<T>>;

template<class ArrItem,
         class Context,
         class Bed,
         class StopCond = DefaultStopCondition>
Domain::ExPolygons calculate_nfp(const ArrItem &itm,
                                 const Context &context,
                                 const Bed &bed,
                                 StopCond stopcond = {})
{
    return NFPArrangeItemTraits<ArrItem>::calculate_nfp(itm, context, bed,
                                                        std::move(stopcond));
}

template<class ArrItem> Domain::Vec2crd reference_vertex(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::reference_vertex(itm);
}

template<class ArrItem> Domain::BoundingBox2crd envelope_bounding_box(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::envelope_bounding_box(itm);
}

template<class ArrItem> Domain::BoundingBox2crd fixed_bounding_box(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::fixed_bounding_box(itm);
}

template<class ArrItem> decltype(auto) envelope_convex_hull(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::envelope_convex_hull(itm);
}

template<class ArrItem> decltype(auto) fixed_convex_hull(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::fixed_convex_hull(itm);
}

template<class ArrItem> decltype(auto) envelope_outline(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::envelope_outline(itm);
}

template<class ArrItem> decltype(auto) fixed_outline(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::fixed_outline(itm);
}

template<class ArrItem> double envelope_area(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::envelope_area(itm);
}

template<class ArrItem> double fixed_area(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::fixed_area(itm);
}

template<class ArrItem> Domain::Vec2crd fixed_centroid(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::fixed_centroid(itm);
}

template<class ArrItem> Domain::Vec2crd envelope_centroid(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::envelope_centroid(itm);
}

template<class ArrItem>
auto allowed_rotations(const ArrItem &itm)
{
    return NFPArrangeItemTraits<ArrItem>::allowed_rotations(itm);
}

template<class It>
Domain::BoundingBox2crd bounding_box(const Range<It> &itms) noexcept
{
    auto pilebb =
        std::accumulate(itms.begin(), itms.end(), Domain::BoundingBox2crd{},
                        [](Domain::BoundingBox2crd bb, const auto &itm) {
                            return Biz::Algorithms::BoundingBox::merge(bb, fixed_bounding_box(itm));
                        });

    return pilebb;
}

template<class It>
Domain::BoundingBox2crd bounding_box_on_bedidx(const Range<It> &itms, int bed_index) noexcept
{
    auto pilebb =
        std::accumulate(itms.begin(), itms.end(), Domain::BoundingBox2crd{},
                        [bed_index](Domain::BoundingBox2crd bb, const auto &itm) {
                            if (bed_index == get_bed_index(itm))
                                return Biz::Algorithms::BoundingBox::merge(bb, fixed_bounding_box(itm));

                            return bb;
                        });

    return pilebb;
}

} // namespace Slic3r::arr2

#endif // ARRANGEITEMTRAITSNFP_HPP
