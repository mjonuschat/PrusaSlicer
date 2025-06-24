#pragma once

#define ENABLE_THUMBNAILS_DEBUG_EXPORT_TO_PNG 0

#if ENABLE_THUMBNAILS_DEBUG_EXPORT_TO_PNG
#include "Slic3r/App/Render/Image.hpp"

namespace Slic3r::App::Scene {

void export_to_png_file(const Render::Images& images, const std::string& path_prefix);

} // namespace Slic3r::App::Scene
#endif // ENABLE_THUMBNAILS_DEBUG_EXPORT_TO_PNG
