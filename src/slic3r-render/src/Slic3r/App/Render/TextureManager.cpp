#include "Slic3r/App/Render/TextureManager.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "libslic3r/Utils.hpp"
#include <Slic3r/Log.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/nowide/fstream.hpp>

namespace Slic3r::App::Render {


Texture* TextureManager::get(const std::string& name, const ImageLoadOptions& opts)
{
    TextureMap::const_iterator it = m_textures.find(name);
    if (it != m_textures.end())
        return it->second.get();

    // Load bitmap
    auto* codec = ImageCodecManager::instance().find_loader(name);
    if (codec == nullptr) {
        SPDLOG_ERROR("Cannot find Image Reader Codec for file {}", name);
        return nullptr;
    }

    std::vector<Image>  images;
    {
        boost::filesystem::path path = boost::filesystem::exists(name) ? name : boost::filesystem::path(resources_dir()) / name;
        boost::nowide::ifstream is(path, std::ios::binary | std::ios::in);
        if (!is.good()) {
            SPDLOG_ERROR("Cannot open file {}", name);
            return nullptr;
        }

        images = codec->load(is, opts);
    }

    if (images.empty()) {
        SPDLOG_ERROR("Cannot load image {}", name);
        return nullptr;
    }

    auto tex = m_device.create_texture();
    for (size_t level = 0; level < images.size(); level++) {
        const auto& img = images[level];
        tex->set_data(img.format(), level, img.width(), img.height(), img.data());
    }

    // TODO: (Optional) Compress bitmap

    m_textures[name] = std::move(tex);
    return m_textures[name].get();
}

Texture* TextureManager::create_empty(const std::string& name, PixelFormat pf, size_t w, size_t h)
{
    TextureMap::const_iterator it = m_textures.find(name);
    if (it != m_textures.end())
        return it->second.get();

    auto tex = m_device.create_texture();
    {
        std::vector<uint8_t> data;
        data.resize(w * h * pixel_format_bytes_per_pixel(pf), 0x7f);
        tex->set_data(pf, 0, w, h, data.data());
    }
    m_textures[name] = std::move(tex);
    return m_textures[name].get();
}

}
