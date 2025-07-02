///|/ Copyright (c) Prusa Research 2022 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef SUPPORTTREESTRATEGIES_HPP
#define SUPPORTTREESTRATEGIES_HPP

#include <memory>

namespace Slic3r { namespace sla {

enum class SupportTreeType { Default, Branching, Organic };
enum class PillarConnectionMode { zigzag, cross, dynamic };

}} // namespace Slic3r::sla

#endif // SUPPORTTREESTRATEGIES_HPP
