#pragma once

#include "libslic3r/IThumbnailImageGenerator.hpp"

#include <memory>

namespace Slic3r::App {

struct SharedThumbnailImageGenerator
{
    std::unique_ptr<Biz::Slicing::IThumbnailImageGenerator> generator;
};

} // namespace Slic3r::App
