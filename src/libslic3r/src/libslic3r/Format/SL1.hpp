///|/ Copyright (c) Prusa Research 2020 - 2022 Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef ARCHIVETRAITS_HPP
#define ARCHIVETRAITS_HPP

#include <string>
#include <memory>

#include "libslic3r/ConfigViews.hpp"
#include "libslic3r/SLA/RasterBase.hpp" // ISlaRasterizer

namespace Slic3r {

/// <summary>
/// Factory for create file rasterizer for the '.sl1' file format
/// </summary>
/// <param name="cfg">Printer configuration like resolution, dimension etc</param>
/// <returns>Copyable(for each layer) rasterizer</returns>
std::unique_ptr<ISlaRasterizer> create_sl1_rasterizer(const SLAPrintConfigView& cfg);
} // namespace Slic3r

#endif // ARCHIVETRAITS_HPP
