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

    Image(PixelFormat format, int w, int h, Data&& data = Data()) noexcept :
        m_width(w),
        m_height(h),
        m_pixel_format(format),
        m_pixels(data)
    {
        const size_t bytes_per_pixel = pixel_format_bytes_per_pixel(format);
        if (m_pixels.empty())
            m_pixels.resize(w * h * bytes_per_pixel, 0);
        DEBUG_ASSERT(m_pixels.size() == w * h * bytes_per_pixel);
    }

    Image(Image&&) noexcept = default;

    ~Image() = default;

    const void* data() const
    {
        return m_pixels.data();
    }

    void* data()
    {
        return m_pixels.data();
    }

    PixelFormat format() const
    {
        return m_pixel_format;
    }

    int width() const
    {
        return m_width;
    }

    int height() const
    {
        return m_height;
    }

    size_t channel_count() const
    {
        return pixel_format_channel_count(m_pixel_format);
    }

    size_t pixel_size() const
    {
        return pixel_format_bytes_per_pixel(m_pixel_format);
    }

    bool is_valid() const
    {
        return m_width > 0
            && m_height > 0
            && pixel_format_bytes_per_pixel(m_pixel_format) * size_t(m_width) * size_t(m_height)
            == m_pixels.size();
    }

    void blit(const Image& source, int x, int y);
    Image half_sampled() const;
    Image rescaled_with_preserved_ratio(const Size& target_size);

    void flip_vertical();

    void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        const size_t n_channels = channel_count();
        size_t idx              = 0;
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
    int m_width{0};
    int m_height{0};
    PixelFormat m_pixel_format{PixelFormat::RGBA8};
    Data m_pixels;
};

using Images = std::vector<Image>;

#define ENABLE_DEBUG_EXPORT_TO_PNG 0
#if ENABLE_DEBUG_EXPORT_TO_PNG
void export_to_png_file(const Render::Image& image, const std::string& path_prefix);
void export_to_png_file(const Render::Images& images, const std::string& path_prefix);
#endif // ENABLE_DEBUG_EXPORT_TO_PNG

} // namespace Slic3r::App::Render
