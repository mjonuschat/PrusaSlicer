///|/ Copyright (c) Prusa Research 2023 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef NFP_HPP
#define NFP_HPP

#include <cstdint>
#include <boost/variant.hpp>
#include <cinttypes>

#include "Slic3r/Domain/ExPolygon.hpp"
#include "Slic3r/Domain/Point.hpp"
#include "Slic3r/Domain/Polygon.hpp"

#include <arrange/Beds.hpp>

namespace Slic3r {

template<int N, class T> using LegacyVec = Domain::Advanced::Vec<T, N>;

template<class Unit = int64_t, class T>
Unit dotperp(const LegacyVec<2, T> &a, const LegacyVec<2, T> &b)
{
    return Unit(a.x()) * Unit(b.y()) - Unit(a.y()) * Unit(b.x());
}

// Convex-Convex nfp in linear time (fixed.size() + movable.size()),
// no memory allocations (if out param is used).
// FIXME: Currently broken for very sharp triangles.
Domain::Polygon nfp_convex_convex(const Domain::Polygon &fixed, const Domain::Polygon &movable);
void nfp_convex_convex(const Domain::Polygon &fixed, const Domain::Polygon &movable, Domain::Polygon &out);
Domain::Polygon nfp_convex_convex_legacy(const Domain::Polygon &fixed, const Domain::Polygon &movable);

Domain::Polygon ifp_convex_convex(const Domain::Polygon &fixed, const Domain::Polygon &movable);

Domain::ExPolygons ifp_convex(const arr2::RectangleBed &bed, const Domain::Polygon &convexpoly);
Domain::ExPolygons ifp_convex(const arr2::CircleBed &bed, const Domain::Polygon &convexpoly);
Domain::ExPolygons ifp_convex(const arr2::IrregularBed &bed, const Domain::Polygon &convexpoly);
inline Domain::ExPolygons ifp_convex(const arr2::InfiniteBed &bed, const Domain::Polygon &convexpoly)
{
    return {};
}

inline Domain::ExPolygons ifp_convex(const arr2::ArrangeBed &bed, const Domain::Polygon &convexpoly)
{
    Domain::ExPolygons ret;
    auto visitor = [&ret, &convexpoly](const auto &b) { ret = ifp_convex(b, convexpoly); };
    boost::apply_visitor(visitor, bed);

    return ret;
}

Domain::Vec2crd reference_vertex(const Domain::Polygon &outline);
Domain::Vec2crd reference_vertex(const Domain::ExPolygon &outline);
Domain::Vec2crd reference_vertex(const Domain::Polygons &outline);
Domain::Vec2crd reference_vertex(const Domain::ExPolygons &outline);

Domain::Vec2crd min_vertex(const Domain::Polygon &outline);

} // namespace Slic3r

#endif // NFP_HPP
