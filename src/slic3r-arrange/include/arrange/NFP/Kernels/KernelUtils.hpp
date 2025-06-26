///|/ Copyright (c) Prusa Research 2023 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef ARRANGEKERNELUTILS_HPP
#define ARRANGEKERNELUTILS_HPP

#include "Slic3r/Domain/Types.hpp"

#include <type_traits>

#include <arrange/NFP/NFPArrangeItemTraits.hpp>
#include <arrange/Beds.hpp>
#include <arrange/DataStoreTraits.hpp>

namespace Slic3r::arr2 {

template<class Itm, class Bed, class Context>
bool find_initial_position(Itm &itm,
                           const Domain::Vec2crd &sink,
                           const Bed &bed,
                           const Context &packing_context)
{
    bool ret = false;

    if constexpr (std::is_convertible_v<Bed, RectangleBed> ||
                  std::is_convertible_v<Bed, InfiniteBed> ||
                  std::is_convertible_v<Bed, CircleBed>)
    {
        if (all_items_range(packing_context).empty()) {
            auto rotations = allowed_rotations(itm);
            set_rotation(itm, 0.);
            auto chull     = envelope_convex_hull(itm);

            for (double rot : rotations) {
                auto chullcpy = chull;
                chullcpy.rotate(rot);
                auto bbitm = Slic3r::bounding_box(chullcpy);

                Domain::Vec2crd cb = sink;
                Domain::Vec2crd ci = Biz::Algorithms::BoundingBox::center(bbitm);

                Domain::Vec2crd d = cb - ci;
                bbitm = Biz::Algorithms::BoundingBox::translated(bbitm, d);

                if (Biz::Algorithms::BoundingBox::contains(bounding_box(bed), bbitm)) {
                    rotate(itm, rot);
                    translate(itm, d);
                    ret = true;
                    break;
                }
            }
        }
    }

    return ret;
}

template<class ArrItem> std::optional<Domain::Vec2crd> get_gravity_sink(const ArrItem &itm)
{
    constexpr const char * SinkKey = "sink";

    std::optional<Domain::Vec2crd> ret;

    auto ptr = get_data<Domain::Vec2crd>(itm, SinkKey);

    if (ptr)
        ret = *ptr;

    return ret;
}

template<class ArrItem> bool is_wipe_tower(const ArrItem &itm)
{
    constexpr const char * Key = "is_wipe_tower";

    return has_key(itm, Key);
}

} // namespace Slic3r::arr2

#endif // ARRANGEKERNELUTILS_HPP
