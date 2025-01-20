#pragma once

#include "Image.hpp"
#include "WithInternal.hpp"

namespace Slic3r::App::Render {

class Context;
class Device;

class Texture : public WithInternal {
public:
    enum class MinFilter { Nearest = 0, Linear};
    enum class MagFilter { Nearest = 0, Linear, MipMap};

    ~Texture() override;

    void set_data(PixelFormat format, size_t level, size_t w, size_t h, const void* data);
    void set_filtering(MinFilter min_filter, MagFilter mag_filter);

private:
    friend class Device;
    explicit Texture(Device& device);
private:
    Device& m_device;
};

}