#include "Image.hpp"

#include <cstring>

namespace Slic3r::App::Render {

size_t pixel_format_bytes_per_pixel(PixelFormat pf)
{
    switch (pf) {
    case PixelFormat::RGB8:
        return 3;
    case PixelFormat::RGBA8:
        return 4;
    default:
        // unsupported format
        assert(false);
        return 0;
    }
}

size_t pixel_format_channel_count(PixelFormat pf)
{
    // as there are only single byte channel values we can use just p

    return pixel_format_bytes_per_pixel(pf);
}


void Image::blit(const Image& source, size_t x, size_t y)
{
    assert(m_pixel_format == source.m_pixel_format);

    const size_t x1 = std::min(m_width, x + source.width());
    const size_t y1 = std::min(m_height, y + source.height());
    const size_t pixel_bytes = pixel_format_bytes_per_pixel(m_pixel_format);
    const size_t blit_row_size = (x1 - x) * pixel_bytes;

    for (size_t yi = y; yi < y1; yi++) {
        const size_t dest_index = (yi * m_width + x) * pixel_bytes;
        const size_t src_index = ((yi - y) * source.width()) * pixel_bytes;
        std::memcpy(&m_pixels[dest_index], &source.m_pixels[src_index], blit_row_size);
    }
}

Image Image::half_sampled() const
{
    Image::Data half_pixels;

    const size_t half_w = m_width / 2;
    const size_t half_h = m_height / 2;
    const size_t channels = channel_count();
    half_pixels.reserve(half_w * half_h * channels);

    assert(channels == pixel_size()); // only byte per channel allowed at the  moment

    const size_t pixel_stride = channels;
    const size_t row_stride = m_width * channels;

    for (size_t y = 0; y < m_height; y += 2) {
        for (size_t x = 0; x < m_width; x += 2) {
            const size_t base_idx = y * row_stride + x * channels;
            for (size_t ch = 0; ch < channels; ch++) {
                uint32_t val = 0;
#pragma unroll
                for (size_t j = 0; j < 2; j++) {
#pragma unroll
                    for (size_t i = 0; i < 2; i++) {
                        val += m_pixels[base_idx + i * pixel_stride + j * row_stride + ch];
                    }
                }
                half_pixels.push_back(val >> 2);
            }
        }
    }

    return {m_pixel_format, half_w, half_h, std::move(half_pixels)};
    
}

}
