#pragma once

#include <string>
#include <unordered_map>

#include "Texture.hpp"
#include "ImageCodec.hpp"

namespace Slic3r::App::Render {

class Context;

class TextureManager {
public:
    explicit TextureManager(Context& context) : m_context(context) {}
    Texture* get(const std::string& name, const ImageLoadOptions& opts = {});
    Texture* create_empty(const std::string& name, PixelFormat pf, size_t w, size_t h);
private:
    using TextureMap = std::unordered_map<std::string, Texture*>; // std::unique_ptr<Texture> ?

    Context& m_context;
    TextureMap m_textures;
};

}