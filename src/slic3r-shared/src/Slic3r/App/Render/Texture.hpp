#pragma once

#include "commonGL.hpp"
#include "Image.hpp"
#include "WithInternal.hpp"

namespace Slic3r::App::Render {

class Context;
class Device;

class Texture : public WithInternal {
public:
    enum class MinFilter { Nearest = 0, Linear};
    enum class MagFilter { Nearest = 0, Linear, MipMap};

    explicit Texture(Device& device);

    void bind(uint8_t unit = 0);
    void unbind();

    void set_data(PixelFormat format, size_t level, size_t w, size_t h, const void* data);
    void set_filtering(MinFilter min_filter, MagFilter mag_filter);

private:
    Device& m_device;
};

}