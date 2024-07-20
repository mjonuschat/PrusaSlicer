#pragma once

#include <string>
#include <unordered_map>

#include "Texture.hpp"
#include "ImageCodec.hpp"

namespace Slic3r::App::Render {

class Context;
class Device;

class TextureManager {
    friend class Context;
    explicit TextureManager(Device& device) : m_device(device) {}
public:
    Texture* get(const std::string& name, const ImageLoadOptions& opts = {});
    Texture* create_empty(const std::string& name, PixelFormat pf, size_t w, size_t h);
private:
    using TextureMap = std::unordered_map<std::string, Texture*>; // std::unique_ptr<Texture> ?

    Device& m_device;
    TextureMap m_textures;
};

}