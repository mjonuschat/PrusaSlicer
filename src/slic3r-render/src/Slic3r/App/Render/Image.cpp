#include "Slic3r/App/Render/Image.hpp"

#include <cstring>
#include "Slic3r/Assert.hpp"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

namespace Slic3r::App::Render {

size_t pixel_format_bytes_per_pixel(PixelFormat pf)
{
    switch (pf) {
    case PixelFormat::RGB8:
        return 3;
    case PixelFormat::RGBA8:
        return 4;
    case PixelFormat::R32F:
        return 4;
    case PixelFormat::R32UI:
        return 4;
    case PixelFormat::RGBA32F:
        return 16;
    case PixelFormat::RGBA16F:
        return 8;
    case PixelFormat::RGB32F:
        return 12;
    case PixelFormat::DepthComponent:
        return 4;
    default:
        // unsupported format
        PANIC("Unsupported pixel format");
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
    ASSERT(m_pixel_format == source.m_pixel_format);

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

    const size_t half_w = std::max<size_t>(1, m_width / 2);
    const size_t half_h = std::max<size_t>(1, m_height / 2);
    const size_t channels = channel_count();
    ASSERT(channels == pixel_size()); // only byte per channel allowed at the  moment

    half_pixels.resize(half_w * half_h * channels);
    stbir_resize_uint8_linear(m_pixels.data(), m_width, m_height, m_width * channels,
        half_pixels.data(), half_w, half_h, half_w * channels, STBIR_RGBA);
    return {m_pixel_format, half_w, half_h, std::move(half_pixels)};    
}

/*
* This function ensures that the aspect ratio is preserved when scaling,
* and if the source and target sizes have different proportions,
* it centers the image with transparent padding (assuming an RGBA format for transparency).
*/
Image Image::rescaled_with_preserved_ratio(size_t target_w, size_t target_h)
{
    const size_t pixel_stride = channel_count();
    Image::Data result(target_w * target_h * pixel_stride, 0); // Initialize with transparent pixels

    size_t src_w = m_width;
    size_t src_h = m_height;

    float scale_x = (float)target_w / src_w;
    float scale_y = (float)target_h / src_h;
    float scale = std::min(scale_x, scale_y);  // Choose the smaller scale to fill the box
    int new_w = int(src_w * scale);
    int new_h = int(src_h * scale);

    std::vector<unsigned char> resized_rgba(new_w * new_h * pixel_stride);
    stbir_resize_uint8_linear(m_pixels.data(), src_w, src_h, pixel_stride * src_w,
        resized_rgba.data(), new_w, new_h, pixel_stride * new_w,
        STBIR_RGBA);

    ASSERT(target_w >= new_w && target_h >= new_h); // only byte per channel allowed at the  moment

    size_t offset_x = (target_w - new_w) / 2; // Center horizontally
    size_t offset_y = (target_h - new_h) / 2; // Center vertically

    for (size_t y = 0; y < new_h; y++) {
        std::memcpy(result.data() + ((y + offset_y) * target_w + offset_x) * pixel_stride,
                    resized_rgba.data() + y * new_w * pixel_stride,
                    new_w * pixel_stride);
        // Ensure transparent padding if using RGBA
        for (size_t x = 0; x < new_w; x++)
            if (pixel_stride == 4) {
                result[((y + offset_y) * target_w + (x + offset_x)) * 4 + 3] = 255; // Set alpha to full opacity
            }
    }

    return { m_pixel_format, target_w, target_h, std::move(result) };
}

}
