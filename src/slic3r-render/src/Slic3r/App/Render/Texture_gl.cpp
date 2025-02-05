#include "Slic3r/App/Render/Texture.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/GL/commonGL.hpp"
#include "Slic3r/App/Render/GL/GLTypes.hpp"
#include "Slic3r/App/Render/GL/GLTextureInternal.hpp"
#include "Slic3r/App/Render/GL/GLDeviceInternal.hpp"

namespace Slic3r::App::Render {

Texture::Texture(Device& device)
    : WithInternal::WithInternal(InternalType<GL::GLTextureInternal>()), m_device(device)
{
    glGenTextures(1, &get_internal_as<GL::GLTextureInternal>().m_id);
    glCheck();
}

Texture::~Texture()
{
    glDeleteTextures(1, &get_internal_as<GL::GLTextureInternal>().m_id);
    glCheck();
}

void Texture::set_data(PixelFormat format, size_t level, size_t w, size_t h, const void* data)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_texture(0, *this);

    GLenum gl_internal_format = GL::texture_internal_format(format);
    GLenum gl_format = GL::texture_format(format);
    GLenum gl_type = GL::texture_format_type(format);
    GLenum gl_target = get_internal_as<GL::GLTextureInternal>().m_target;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glCheck();
    glTexImage2D(gl_target, level, gl_internal_format, w, h, 0, gl_format, gl_type, data);
    glCheck();
    glTexParameteri(gl_target, GL_TEXTURE_MAX_LEVEL, level);
    glCheck();
}

void Texture::set_filtering(TextureMinFilter min_filter, TextureMagFilter mag_filter)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_texture(0, *this);
    GLenum gl_target = get_internal_as<GL::GLTextureInternal>().m_target;
    glTexParameteri(gl_target, GL_TEXTURE_MIN_FILTER, GL::type(min_filter));
    glCheck();
    glTexParameteri(gl_target, GL_TEXTURE_MAG_FILTER, GL::type(mag_filter));
    glCheck();
}

}
