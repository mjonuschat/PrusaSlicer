#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

#include "Types.hpp"

#include "Slic3r/Assert.hpp"


namespace Slic3r::App::Render {

size_t pixel_format_bytes_per_pixel(PixelFormat pf);
size_t pixel_format_channel_count(PixelFormat pf);

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
        const size_t bytes_per_pixel = pixel_format_bytes_per_pixel(format);
        if (m_pixels.empty())
            m_pixels.resize(w * h * bytes_per_pixel, 0);
        DEBUG_ASSERT(data.size() == w * h * bytes_per_pixel);
    }

    Image(Image&&) noexcept = default;

    ~Image() = default;

    const void* data() const { return m_pixels.data(); }
    void* data() { return m_pixels.data(); }

    PixelFormat format() const { return m_pixel_format; }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }

    size_t channel_count() const { return pixel_format_channel_count(m_pixel_format); }
    size_t pixel_size() const { return pixel_format_bytes_per_pixel(m_pixel_format); }

    void blit(const Image& source, size_t x, size_t y);
    Image half_sampled() const;
    Image rescaled_with_preserved_ratio(size_t target_w, size_t target_h);

    void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        const size_t n_channels = channel_count();
        size_t idx = 0;
        for (auto& ch : m_pixels) {
            switch (idx) {
            case 0:
                ch = r;
                break;

            case 1:
                ch = g;
                break;

            case 2:
                ch = b;
                break;

            default:
                ch = a;
                break;
            }
            idx = (idx + 1) % n_channels;
        }
    }

private:
    size_t m_width{0};
    size_t m_height{0};
    PixelFormat m_pixel_format{PixelFormat::RGBA8};
    Data m_pixels;
};

} // namespace Slic3r::App::Render