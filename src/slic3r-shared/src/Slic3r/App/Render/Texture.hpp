#pragma once

#include "commonGL.hpp"
#include "Image.hpp"

namespace Slic3r::App::Render {

class Texture {
public:
    Texture();

    void bind(uint8_t unit = 0);
    void unbind();

    void set_data(PixelFormat format, size_t level, size_t w, size_t h, const void* data);

private:
    GLuint m_id{0};
    static constexpr uint8_t UNBOUND = 255;
    uint8_t m_bound_unit{UNBOUND};
};

}