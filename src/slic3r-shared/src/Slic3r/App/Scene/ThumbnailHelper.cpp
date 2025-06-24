#include "Slic3r/App/Scene/ThumbnailHelper.hpp"

#if ENABLE_THUMBNAILS_DEBUG_EXPORT_TO_PNG

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <filesystem>

namespace Slic3r::App::Scene {

void export_to_png_file(const Render::Images& images, const std::string& path_prefix)
{
    for (const auto& image : images) {
        int w = image.width();
        int h = image.height();
        int comp = int(image.channel_count());
        int stride_bytes = int(w * image.pixel_size());
        std::string filename = path_prefix + "_" + std::to_string(w) + "_" + std::to_string(h) + ".png";

        std::filesystem::path out(filename);
        out.remove_filename();
        if (!std::filesystem::exists(out))
            std::filesystem::create_directories(out);

        if (stbi_write_png(filename.c_str(), w, h, comp, image.data(), stride_bytes) == 0)
            PANIC("Unable to save thumbnail to file: " + filename);
    }
}

} // namespace Slic3r::App::Scene

#endif // ENABLE_THUMBNAILS_DEBUG_EXPORT_TO_PNG
