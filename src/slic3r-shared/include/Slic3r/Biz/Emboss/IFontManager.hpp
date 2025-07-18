#pragma once
#include <memory>
#include "Slic3r/Domain/TextConfiguration.hpp" // FontList + FontDescriptor
#include "Slic3r/Domain/FontFile.hpp"

namespace Slic3r::Biz::Emboss {
/// <summary>
/// Provide access to font
/// </summary>
class IFontManager
{
public:
    virtual ~IFontManager() = default;

    /// <summary>
    /// List of OS installed fonts,
    /// where hope it will be possible to triangulate glyph shapes.
    /// NOTE: (not CONST) Check OS changes and cache openable font list.
    /// </summary>
    /// <returns>Current available fonts</returns>
    virtual const Domain::FontList& get_fonts() = 0;

    /// <summary>
    /// Open File with font defined by descriptor
    /// NOTE: (not CONST) Update list of openable font descriptors
    /// </summary>
    /// <param name="descriptor">Define font (glyph shapes)</param>
    /// <returns>Opened file</returns>
    virtual std::unique_ptr<const Domain::FontFile> open(const Domain::FontDescriptor& descriptor) = 0;

    /// <summary>
    /// Getter on current font descriptor type
    /// To be able distiquish wheather descriptor was created on system with same creator
    /// </summary>
    /// <returns>Current type</returns>
    virtual Domain::FontDescriptor::Type get_current_type() const = 0;

    /// <summary>
    /// create cca 5 descriptors for OS favorit fonts
    /// NOTE: (not CONST) internaly call get_fonts()
    /// </summary>
    /// <returns>Favorit fonts</returns>
    virtual Domain::FontList create_favorit() = 0;
};
} // namespace Slic3r::Biz::Emboss
