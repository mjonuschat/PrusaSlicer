#pragma once

#include <cstdint>
#include "Slic3r/Domain/Image.hpp"
#include "Slic3r/Domain/Size.hpp"

namespace Slic3r::Biz::Algorithms::ImageUtils {
bool is_valid(const Domain::Image& image);

void blit(Domain::Image& destination, const Domain::Image& source, int x, int y);
Domain::Image half_sampled(const Domain::Image& image);
Domain::Image rescaled_with_preserved_ratio(const Domain::Image& image, const Domain::Size& target_size);

void flip_vertical(Domain::Image& image);

void fill(Domain::Image& image, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
} // namespace Slic3r::Biz::Algorithms::ImageUtils
