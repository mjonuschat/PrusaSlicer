///|/ Copyright (c) Prusa Research 2020 - 2022 Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef ARCHIVETRAITS_HPP
#define ARCHIVETRAITS_HPP

#include <string>
#include <memory>

#include "libslic3r/SLA/RasterBase.hpp" // ISlaRasterizer
#include "libslic3r/SLA/SLAResult.hpp"

namespace Slic3r {

/// <summary>
/// File writer for the '.sl1' file format, store slicing result into file
/// NOTE: Throw exception in case can't write to file_path
/// e.g. no space, no privilege, bad network, ...
/// </summary>
/// <param name="file_path">File path for store result</param>
/// <param name="data">Already rasterized and encoded files with configurations and thumbnails</param>
void store_sl1(const std::string& file_path, const Biz::Slicing::SLAResult& data);

/// <summary>
/// Factory for create file rasterizer for the '.sl1' file format
/// </summary>
/// <param name="cfg">Printer configuration like resolution, dimension etc</param>
/// <returns>Copyable(for each layer) rasterizer</returns>
std::unique_ptr<ISlaRasterizer> create_sl1_rasterizer(const SLAPrintConfigView& cfg);
} // namespace Slic3r

#endif // ARCHIVETRAITS_HPP
