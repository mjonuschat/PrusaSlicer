#pragma once

#include <cassert>
#include <vector>

namespace Slic3r::App::Render {

enum class PixelFormat
{
    RGB8 = 0,
    RGBA8,
};

size_t pixel_format_bytes_per_pixel(PixelFormat pf);

/**
 * Simple bitmap container storing image dimensions, pixel format and pixels themselves.
 * Use ImageCodecManager to get image codec to load image.
 */
class Image
{
public:
    using Data = std::vector<uint8_t>;

    Image(PixelFormat format, size_t w, size_t h, Data&& data = Data()) noexcept
        : m_width(w), m_height(h), m_pixel_format(format), m_pixels(data)
    {
        assert(data.size() == w * h * pixel_format_bytes_per_pixel(format));
    }
    Image(Image&&) noexcept = default;

    ~Image() = default;

    const void* data() const { return m_pixels.data(); }
    PixelFormat format() const { return m_pixel_format; }
    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

    void blit(const Image& source, size_t x, size_t y);

private:
    size_t m_width{0};
    size_t m_height{0};
    PixelFormat m_pixel_format{PixelFormat::RGBA8};
    Data m_pixels;
};

} // namespace Slic3r::App::Render