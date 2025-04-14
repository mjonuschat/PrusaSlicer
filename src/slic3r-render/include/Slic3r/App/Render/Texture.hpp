#pragma once

#include "Image.hpp"
#include "WithInternal.hpp"

namespace Slic3r::App::Render {

class Device;

class Texture : public WithInternal {
public:
    ~Texture() override;

    void set_data(PixelFormat format, size_t level, size_t w, size_t h, const void* data);
    void set_sub_data(PixelFormat format, size_t level, size_t offset_x, size_t offset_y, size_t w, size_t h, const void* data);
    void set_filtering(TextureMinFilter min_filter, TextureMagFilter mag_filter);

    void set_object_name(const std::string& object_name);
    void set_wrap_s(TextureWrap wrap);
    void set_wrap_t(TextureWrap wrap);
    void set_wrap_r(TextureWrap wrap);
    void set_border_color(const std::array<float, 4>& color);

private:
    friend class Device;
    explicit Texture(Device& device);
private:
    Device& m_device;
};

}
