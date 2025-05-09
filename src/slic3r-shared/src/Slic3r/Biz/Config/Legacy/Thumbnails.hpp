///|/ Copyright (c) Prusa Research 2022 Lukáš Hejl @hejllukas, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_GCodeThumbnailslegacy_hpp_
#define slic3r_GCodeThumbnailslegacy_hpp_

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <boost/format.hpp>
#include <vector>
#include <memory>
#include <string_view>
#include <algorithm>
#include <string>
#include <utility>

#include "Point.hpp"
#include "PrintConfig.hpp"
#include "enum_bitmask.hpp"

namespace Slic3rLegacy {
class ConfigBase;

    enum class ThumbnailError : int { InvalidVal, OutOfRange, InvalidExt };
    using ThumbnailErrors = enum_bitmask<ThumbnailError>;
    ENABLE_ENUM_BITMASK_OPERATORS(ThumbnailError);
}

namespace Slic3rLegacy::GCodeThumbnails {

struct CompressedImageBuffer
{
    void       *data { nullptr };
    size_t      size { 0 };
    virtual ~CompressedImageBuffer() {}
    virtual std::string_view tag() const = 0;
};

typedef std::vector<std::pair<GCodeThumbnailsFormat, Vec2d>> GCodeThumbnailDefinitionsList;

using namespace std::literals;
std::pair<GCodeThumbnailDefinitionsList, ThumbnailErrors> make_and_check_thumbnail_list(const std::string& thumbnails_string, const std::string_view def_ext = "PNG"sv);
std::pair<GCodeThumbnailDefinitionsList, ThumbnailErrors> make_and_check_thumbnail_list(const ConfigBase &config);

std::string get_error_string(const ThumbnailErrors& errors);


} // namespace Slic3r::GCodeThumbnails

#endif // slic3r_GCodeThumbnails_hpp_
