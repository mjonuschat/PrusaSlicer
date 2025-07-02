///|/ Copyright (c) Prusa Research 2022 - 2023 Tomáš Mészáros @tamasmeszaros
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef SLAARCHIVEREADER_HPP
#define SLAARCHIVEREADER_HPP

#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/ExPolygon.hpp"

struct indexed_triangle_set;

namespace Slic3r {

// A generic indicator for the quality of an imported model. Obviously, the
// original cannot be fully reconstructed.
enum class SLAImportQuality { Accurate, Balanced, Fast };

// Raised when the needed metadata cannot be retrieved or guessed from an archive
class MissingProfileError : public RuntimeError
{
    using RuntimeError::RuntimeError;
};

// A shortname for status indication function.
// The argument is the status (from <0, 100>)
// Returns false if cancel was requested.
using ProgrFn = std::function<bool(int)>;

// Abstract interface for an archive reader. This needs to be implemented for
// every supported archive format.
class SLAArchiveReader {
public:

    virtual ~SLAArchiveReader() = default;

    // Read the profile and reconstruct the slices
    virtual ConfigSubstitutions read(std::vector<ExPolygons> &slices,
                                     DynamicPrintConfig      &profile) = 0;

    // Overload for reading only the profile contained in the archive (if present)
    virtual ConfigSubstitutions read(DynamicPrintConfig &profile) = 0;

    // Creates a reader instance based on the provided file path.
    // format_id can be one of the archive type identifiers returned by
    // registered_archives(). If left empty, only the file extension will
    // be considered. If more archive types have the same extension (like *.zip)
    // The first match is used.
    static std::unique_ptr<SLAArchiveReader> create(
        const std::string &fname,
        const std::string &format_id,
        SLAImportQuality   quality = SLAImportQuality::Balanced,
        const ProgrFn     &progr   = [](int) { return false; });
};

// Raised in import_sla_archive when a nullptr reader is returned by
// SLAArchiveReader::create()
class ReaderUnimplementedError : public RuntimeError
{
    using RuntimeError::RuntimeError;
};

// Helper free functions to import an archive using the above interface.
// Can throw ReaderUnimplementedError or MissingProfileError
ConfigSubstitutions import_sla_archive(
    const std::string       &zipfname,
    const std::string       &format_id,
    indexed_triangle_set    &out,
    DynamicPrintConfig      &profile,
    SLAImportQuality         quality = SLAImportQuality::Balanced,
    const ProgrFn &progr = [](int) { return true; });

// Only reads the profile, doesn't reconstruct the model.
ConfigSubstitutions import_sla_archive(const std::string  &zipfname,
                                       const std::string  &format_id,
                                       DynamicPrintConfig &out);

} // namespace Slic3r

#endif // SLAARCHIVEREADER_HPP
