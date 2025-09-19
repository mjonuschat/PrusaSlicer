#pragma once

#include <cstdint>
#include "Slic3r/Domain/Image.hpp"
#include "Slic3r/Domain/Size.hpp"

namespace Slic3r::Biz::Algorithms::ImageUtils {
bool is_valid(const Domain::Image& image);

void blit(Domain::Image& destination, const Domain::Image& source, int x, int y);
Domain::Image half_sampled(const Domain::Image& image);
Domain::Image rescaled_with_preserved_ratio(const Domain::Image& image, const Domain::Size& target_size);

/**
  * @brief Compress the given image.
  * @param image The image to compress.
  * @note The image format MUST be either PixelFormat::RGB8 or PixelFormat::RGBA8.
  *       The output image format will be:
  *       in PixelFormat::RGB8  -> out PixelFormat::RGB_DXT1.
  *       in PixelFormat::RGBA8 -> out PixelFormat::RGBA_DXT5.
  */
Domain::Image compress(const Domain::Image& image);

void flip_vertical(Domain::Image& image);

void fill(Domain::Image& image, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

#define ENABLE_DEBUG_EXPORT_TO_PNG 0
#if ENABLE_DEBUG_EXPORT_TO_PNG
void export_to_png_file(const Slic3r::Domain::Image& image, const std::string& path_prefix);
void export_to_png_file(const Slic3r::Domain::Images& images, const std::string& path_prefix);
#endif // ENABLE_DEBUG_EXPORT_TO_PNG

} // namespace Slic3r::Biz::Algorithms::ImageUtils
