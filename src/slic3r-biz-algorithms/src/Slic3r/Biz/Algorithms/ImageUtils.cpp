#include "Slic3r/Biz/Algorithms/ImageUtils.hpp"
#include <cstring>
#include <algorithm>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace Slic3r::Biz::Algorithms::ImageUtils {

using Domain::Image;
using Domain::Images;
using Domain::PixelFormat;
using Domain::Size;
using Domain::Sizes;

size_t pixel_format_channel_count(PixelFormat pf)
{
    // as there are only single byte channel values we can use just p
    return pixel_format_bytes_per_pixel(pf);
}

std::size_t channel_count(const Image& image)
{
    return pixel_format_channel_count(image.format());
}

std::size_t pixel_size(const Image& image)
{
    return pixel_format_bytes_per_pixel(image.format());
}

bool is_valid(const Image& image)
{
    return image.width() > 0
        && image.height() > 0
        && pixel_format_bytes_per_pixel(image.format())
            * size_t(image.width())
            * size_t(image.height())
        == image.pixels.size();
}

void blit(Image& destination, const Image& source, int x, int y)
{
    ASSERT(destination.format() == source.format());

    const int x1               = std::min(destination.width(), x + source.width());
    const int y1               = std::min(destination.height(), y + source.height());
    const size_t pixel_bytes   = pixel_format_bytes_per_pixel(destination.format());
    const size_t blit_row_size = (x1 - x) * pixel_bytes;

    for (size_t yi = y; yi < y1; yi++) {
        const size_t dest_index = (yi * destination.width() + x) * pixel_bytes;
        const size_t src_index  = ((yi - y) * source.width()) * pixel_bytes;
        std::memcpy(&destination.pixels[dest_index], &source.pixels[src_index], blit_row_size);
    }
}

Image half_sampled(const Image& image)
{
    Image::Data half_pixels;

    const int half_w      = std::max(1, image.width() / 2);
    const int half_h      = std::max(1, image.height() / 2);
    const size_t channels = channel_count(image);
    half_pixels.reserve(half_w * half_h * channels);

    ASSERT(channels == pixel_size(image)); // only byte per channel allowed at the  moment

    half_pixels.resize(half_w * half_h * channels);
    stbir_resize_uint8_linear(
        image.pixels.data(),
        image.width(),
        image.height(),
        image.width() * channels,
        half_pixels.data(),
        half_w,
        half_h,
        half_w * channels,
        STBIR_RGBA
    );
    return {image.format(), half_w, half_h, std::move(half_pixels)};
}

/*
 * This function ensures that the aspect ratio is preserved when scaling,
 * and if the source and target sizes have different proportions,
 * it centers the image with transparent padding (assuming an RGBA format for transparency).
 */
Image rescaled_with_preserved_ratio(const Image& image, const Size& target_size)
{
    const size_t pixel_stride = channel_count(image);
    Image::Data result(
        target_size.width * target_size.height * pixel_stride,
        0
    ); // Initialize with transparent pixels

    size_t src_w = image.width();
    size_t src_h = image.height();

    Size size{image.width(), image.height()};
    size.scale(target_size, Size::ScaleMode::KeepAspectRatio);

    std::vector<unsigned char> resized_rgba(size.width * size.height * pixel_stride);
    stbir_resize_uint8_linear(
        image.pixels.data(),
        src_w,
        src_h,
        pixel_stride * src_w,
        resized_rgba.data(),
        size.width,
        size.height,
        pixel_stride * size.width,
        STBIR_RGBA
    );

    ASSERT(
        target_size.width >= size.width && target_size.height >= size.height
    ); // only byte per channel allowed at the  moment

    size_t offset_x = (target_size.width - size.width) / 2; // Center horizontally
    size_t offset_y = (target_size.height - size.height) / 2; // Center vertically

    for (size_t y = 0; y < size.height; y++) {
        std::memcpy(
            result.data() + ((y + offset_y) * target_size.width + offset_x) * pixel_stride,
            resized_rgba.data() + y * size.width * pixel_stride,
            size.width * pixel_stride
        );
        // Ensure transparent padding if using RGBA
        for (size_t x = 0; x < size.width; x++) {
            if (pixel_stride == 4) {
                result[((y + offset_y) * target_size.width + (x + offset_x)) * 4 + 3] = 255; // Set alpha to full opacity
            }
        }
    }

    return {image.format(), target_size.width, target_size.height, std::move(result)};
}

void flip_vertical(Image& image)
{
    size_t pixel_bytes = pixel_format_bytes_per_pixel(image.format());
    size_t row_size    = image.width() * pixel_bytes;
    size_t half_height = image.height() / 2;
    for (size_t y = 0; y < half_height; ++y) {
        size_t top_index    = y * row_size;
        size_t bottom_index = (image.height() - 1 - y) * row_size;
        std::swap_ranges(
            image.pixels.begin() + top_index,
            image.pixels.begin() + top_index + row_size,
            image.pixels.begin() + bottom_index
        );
    }
}

#if ENABLE_DEBUG_EXPORT_TO_PNG
void export_to_png_file(const Image& image, const std::string& path_prefix)
{
    int w                = image.width();
    int h                = image.height();
    int comp             = int(image.channel_count());
    int stride_bytes     = int(w * image.pixel_size());
    std::string filename = path_prefix + "_" + std::to_string(w) + "_" + std::to_string(h) + ".png";

    std::filesystem::path out(filename);
    out.remove_filename();
    if (!std::filesystem::exists(out))
        std::filesystem::create_directories(out);

    if (stbi_write_png(filename.c_str(), w, h, comp, image.data(), stride_bytes) == 0)
        PANIC("Unable to save thumbnail to file: " + filename);
}

void export_to_png_file(const Images& images, const std::string& path_prefix)
{
    for (const auto& image : images) {
        export_to_png_file(image, path_prefix);
    }
}
#endif // ENABLE_DEBUG_EXPORT_TO_PNG

void fill(Image& image, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    const size_t n_channels = channel_count(image);
    size_t idx              = 0;
    for (auto& ch : image.pixels) {
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

} // namespace Slic3r::Biz::Algorithms::ImageUtils
