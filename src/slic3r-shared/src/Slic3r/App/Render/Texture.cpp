#include "Texture.hpp"

namespace Slic3r::App::Render {

namespace GL {
GLenum texture_format(PixelFormat format)
{
    switch (format) {
    case PixelFormat::RGB8:
        return GL_RGB;
        
    case PixelFormat::RGBA8:
        return GL_RGBA;
    
    default:
        // Unsupported format
        assert(false);
    }
}

GLenum texture_format_type(PixelFormat format)
{
    switch (format) {
    case PixelFormat::RGB8:
    case PixelFormat::RGBA8:
        return GL_UNSIGNED_BYTE;
    default:
        // Unsupported format
        assert(false);
        return GL_UNSIGNED_BYTE;
    }
}
}

Texture::Texture()
{
    glGenTextures(1, &m_id);
    glCheck();
}

void Texture::bind(uint8_t unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glCheck();
    m_bound_unit = unit;
}

void Texture::unbind()
{
    glActiveTexture(GL_TEXTURE0 + m_bound_unit);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_bound_unit = UNBOUND;
    glCheck();
}


void Texture::set_data(PixelFormat format,size_t level, size_t w, size_t h, const void* data)
{
    bind(0);
    GLenum gl_format = GL::texture_format(format);
    GLenum gl_type = GL::texture_format_type(format);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glCheck();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheck();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glCheck();
    glTexImage2D(GL_TEXTURE_2D, level, gl_format, w, h, 0, gl_format, gl_type, data);
    glCheck();
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
//    glCheck();
}


}
