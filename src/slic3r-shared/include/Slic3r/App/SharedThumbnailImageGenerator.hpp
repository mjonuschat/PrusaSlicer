#pragma once

#include "Slic3r/Biz/IThumbnailImageGenerator.hpp"

#include <memory>

namespace Slic3r::App {

struct SharedThumbnailImageGenerator
{
    std::unique_ptr<Biz::IThumbnailImageGenerator> generator;
};

} // namespace Slic3r::App
