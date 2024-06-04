#include "Image.hpp"

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

}
