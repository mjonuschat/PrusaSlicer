#pragma once

#include "commonGL.hpp"
#include "Image.hpp"

namespace Slic3r::App::Render {

class Context;

class Texture {
public:
    enum class MinFilter { Nearest = 0, Linear};
    enum class MagFilter { Nearest = 0, Linear, MipMap};

    explicit Texture(Context& context);

    void bind(uint8_t unit = 0);
    void unbind();

    void set_data(PixelFormat format, size_t level, size_t w, size_t h, const void* data);
    void set_filtering(MinFilter min_filter, MagFilter mag_filter);

private:
    Context& m_context;
    GLuint m_id{0};
    static constexpr uint8_t UNBOUND = 255;
    uint8_t m_bound_unit{UNBOUND};
};

}