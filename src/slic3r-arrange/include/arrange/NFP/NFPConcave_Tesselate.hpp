///|/ Copyright (c) Prusa Research 2023 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef NFPCONCAVE_TESSELATE_HPP
#define NFPCONCAVE_TESSELATE_HPP

#include "Slic3r/Domain/ExPolygon.hpp"
#include "Slic3r/Domain/Polygon.hpp"

namespace Slic3r {

Domain::Polygons convex_decomposition_tess(const Domain::Polygon &expoly);
Domain::Polygons convex_decomposition_tess(const Domain::ExPolygon &expoly);
Domain::Polygons convex_decomposition_tess(const Domain::ExPolygons &expolys);
Domain::ExPolygons nfp_concave_concave_tess(const Domain::ExPolygon &fixed, const Domain::ExPolygon &movable);

} // namespace Slic3r

#endif // NFPCONCAVE_TESSELATE_HPP
