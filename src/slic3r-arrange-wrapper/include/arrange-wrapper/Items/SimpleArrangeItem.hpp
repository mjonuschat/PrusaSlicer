///|/ Copyright (c) Prusa Research 2023 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef SIMPLEARRANGEITEM_HPP
#define SIMPLEARRANGEITEM_HPP

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Slic3r/Biz/Algorithms/ClipperUtils.hpp"
#include "Slic3r/Biz/Algorithms/Geometry/ConvexHull.hpp"
#include "Slic3r/Biz/Algorithms/Polygon.hpp"
#include "Slic3r/Domain/BoundingBox.hpp"
#include "Slic3r/Domain/ExPolygon.hpp"
#include "Slic3r/Domain/ObjectID.hpp"
#include "Slic3r/Domain/Polygon.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <arrange/ArrangeItemTraits.hpp>
#include <arrange/PackingContext.hpp>
#include <arrange/NFP/NFPArrangeItemTraits.hpp>
#include <arrange/NFP/NFP.hpp>

#include <arrange-wrapper/Arrange.hpp>
#include <arrange-wrapper/Tasks/FillBedTask.hpp>
#include <arrange-wrapper/Tasks/ArrangeTask.hpp>
#include <arrange-wrapper/Items/MutableItemTraits.hpp>


namespace Slic3r::arr2 {
struct InfiniteBed;

class SimpleArrangeItem {
    Domain::Polygon m_shape;

    Domain::Vec2crd m_translation = Domain::Vec2crd::Zero();
    double  m_rotation = 0.;
    int     m_priority = 0;
    int     m_bed_idx = Unarranged;
    std::optional<int> m_bed_constraint;

    std::vector<double> m_allowed_rotations = {0.};
    Domain::ObjectID m_obj_id;

public:
    explicit SimpleArrangeItem(Domain::Polygon chull = {}): m_shape{std::move(chull)} {}

    void set_shape(Domain::Polygon chull) { m_shape = std::move(chull); }

    const Domain::Vec2crd& get_translation() const noexcept { return m_translation; }
    double get_rotation() const noexcept { return m_rotation; }
    int get_priority() const noexcept { return m_priority; }
    int get_bed_index() const noexcept { return m_bed_idx; }
    std::optional<int> get_bed_constraint() const noexcept {
        return m_bed_constraint;
    }

    void set_translation(const Domain::Vec2crd &v) { m_translation = v; }
    void set_rotation(double v) noexcept { m_rotation = v; }
    void set_priority(int v) noexcept { m_priority = v; }
    void set_bed_index(int v) noexcept { m_bed_idx = v; }
    void set_bed_constraint(std::optional<int> v) noexcept { m_bed_constraint = v; }

    const Domain::Polygon &shape() const { return m_shape; }
    Domain::Polygon outline() const;

    const auto &allowed_rotations() const noexcept
    {
        return m_allowed_rotations;
    }

    void set_allowed_rotations(std::vector<double> rots)
    {
        m_allowed_rotations = std::move(rots);
    }

    void set_object_id(const Domain::ObjectID &id) noexcept { m_obj_id = id; }
    const Domain::ObjectID & get_object_id() const noexcept { return m_obj_id; }
};

template<> struct NFPArrangeItemTraits_<SimpleArrangeItem>
{
    template<class Context, class Bed, class StopCond>
    static Domain::ExPolygons calculate_nfp(const SimpleArrangeItem &item,
                                            const Context &packing_context,
                                            const Bed &bed,
                                            StopCond &&stop_cond)
    {
        auto fixed_items = all_items_range(packing_context);
        Domain::Polygons nfps;
        nfps.reserve(fixed_items.size());
        for (const SimpleArrangeItem &fixed_part : fixed_items) {
            Domain::Polygon subnfp = nfp_convex_convex_legacy(fixed_part.outline(),
                                                      item.outline());
            nfps.emplace_back(subnfp);


            if (stop_cond()) {
                nfps.clear();
                break;
            }
        }

        Domain::ExPolygons nfp_ex;
        if (!stop_cond()) {
            if constexpr (!std::is_convertible_v<Bed, InfiniteBed>) {
                Domain::ExPolygons ifpbed = ifp_convex(bed, item.outline());
                nfp_ex = Biz::Algorithms::ClipperUtils::diff_ex(ifpbed, nfps);
            } else {
                nfp_ex = Biz::Algorithms::ClipperUtils::union_ex(nfps);
            }
        }

        return nfp_ex;
    }

    static Domain::Vec2crd reference_vertex(const SimpleArrangeItem &item)
    {
        return Slic3r::reference_vertex(item.outline());
    }

    static Domain::BoundingBox2crd envelope_bounding_box(const SimpleArrangeItem &itm)
    {
        return Biz::Algorithms::Polygon::get_extents(itm.outline());
    }

    static Domain::BoundingBox2crd fixed_bounding_box(const SimpleArrangeItem &itm)
    {
        return Biz::Algorithms::Polygon::get_extents(itm.outline());
    }

    static Domain::Polygons envelope_outline(const SimpleArrangeItem &itm)
    {
        return {itm.outline()};
    }

    static Domain::Polygons fixed_outline(const SimpleArrangeItem &itm)
    {
        return {itm.outline()};
    }

    static Domain::Polygon envelope_convex_hull(const SimpleArrangeItem &itm)
    {
        return Biz::Algorithms::Geometry::convex_hull(itm.outline());
    }

    static Domain::Polygon fixed_convex_hull(const SimpleArrangeItem &itm)
    {
        return Biz::Algorithms::Geometry::convex_hull(itm.outline());
    }

    static double envelope_area(const SimpleArrangeItem &itm)
    {
        return itm.shape().area();
    }

    static double fixed_area(const SimpleArrangeItem &itm)
    {
        return itm.shape().area();
    }

    static const auto& allowed_rotations(const SimpleArrangeItem &itm) noexcept
    {
        return itm.allowed_rotations();
    }

    static Domain::Vec2crd fixed_centroid(const SimpleArrangeItem &itm) noexcept
    {
        return itm.outline().centroid();
    }

    static Domain::Vec2crd envelope_centroid(const SimpleArrangeItem &itm) noexcept
    {
        return itm.outline().centroid();
    }
};

template<> struct IsMutableItem_<SimpleArrangeItem>: public std::true_type {};

template<>
struct MutableItemTraits_<SimpleArrangeItem> {

    static void set_priority(SimpleArrangeItem &itm, int p) { itm.set_priority(p); }
    static void set_convex_shape(SimpleArrangeItem &itm, const Domain::Polygon &shape)
    {
        itm.set_shape(shape);
    }
    static void set_shape(SimpleArrangeItem &itm, const Domain::ExPolygons &shape)
    {
        itm.set_shape(Biz::Algorithms::Geometry::convex_hull(shape));
    }
    static void set_convex_envelope(SimpleArrangeItem &itm, const Domain::Polygon &envelope)
    {
        itm.set_shape(envelope);
    }
    static void set_envelope(SimpleArrangeItem &itm, const Domain::ExPolygons &envelope)
    {
        itm.set_shape(Biz::Algorithms::Geometry::convex_hull(envelope));
    }

    template<class T>
    static void set_data(SimpleArrangeItem &itm, const std::string &key, T &&data)
    {}

    static void set_allowed_rotations(SimpleArrangeItem &itm, const std::vector<double> &rotations)
    {
        itm.set_allowed_rotations(rotations);
    }
};

template<> struct ImbueableItemTraits_<SimpleArrangeItem>
{
    static void imbue_id(SimpleArrangeItem &itm, const Domain::ObjectID &id)
    {
        itm.set_object_id(id);
    }

    static std::optional<Domain::ObjectID> retrieve_id(const SimpleArrangeItem &itm)
    {
        std::optional<Domain::ObjectID> ret;
        if (itm.get_object_id().valid())
            ret = itm.get_object_id();

        return ret;
    }
};

extern template class  ArrangeableToItemConverter<SimpleArrangeItem>;
extern template struct ArrangeTask<SimpleArrangeItem>;
extern template struct FillBedTask<SimpleArrangeItem>;
extern template struct MultiplySelectionTask<SimpleArrangeItem>;
extern template class  Arranger<SimpleArrangeItem>;

} // namespace Slic3r::arr2

#endif // SIMPLEARRANGEITEM_HPP
