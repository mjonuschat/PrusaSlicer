#pragma once

#include <cstddef>

namespace Slic3r::Domain {

enum class PixelFormat
{
    RGB8 = 0,
    RGBA8,
    R32F,
    R32UI,
    RGBA32F,
    RGBA16F,
    RGB32F,
    DepthComponent,
};

std::size_t pixel_format_bytes_per_pixel(PixelFormat pf);

} // namespace Slic3r::Domain
