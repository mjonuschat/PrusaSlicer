#include "Slic3r/App/Render/Texture.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/GL/commonGL.hpp"
#include "Slic3r/App/Render/GL/GLTypes.hpp"
#include "Slic3r/App/Render/GL/GLTextureInternal.hpp"
#include "Slic3r/App/Render/GL/GLDeviceInternal.hpp"

namespace Slic3r::App::Render {

int Texture::width() const
{
    return m_width;
}

int Texture::height() const
{
    return m_height;
}

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

void Texture::set_data(PixelFormat format, int level, int w, int h, const void* data)
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

    m_width = w;
    m_height = h;
}

void Texture::set_sub_data(PixelFormat format, int level, int offset_x, int offset_y, int w, int h, const void* data)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_texture(0, *this);
    GLenum gl_target = get_internal_as<GL::GLTextureInternal>().m_target;
    GLenum gl_format = GL::texture_format(format);
    GLenum gl_type = GL::texture_format_type(format);
    glTexSubImage2D(gl_target, 0, offset_x, offset_y, w, h, gl_format, gl_type, data);
    glCheck();

    m_width = w;
    m_height = h;
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


void Texture::set_object_name(const std::string &object_name)
{
    // glObjectLabel is OpenGL 4.3, OpenGL version @ Mac is 4.1
    if (glObjectLabel == nullptr)
        return;
    m_device.get_internal_as<GL::GLDeviceInternal>().bind_texture(0, *this);
    glObjectLabel(GL_TEXTURE, get_internal_as<GL::GLTextureInternal>().m_id, object_name.size(), object_name.data());
    glCheck();
}

void Texture::set_wrap_s(TextureWrap wrap)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_texture(0, *this);
    GLenum gl_target = get_internal_as<GL::GLTextureInternal>().m_target;
    glTexParameteri(gl_target, GL_TEXTURE_WRAP_S, GL::type(wrap));
    glCheck();
}

void Texture::set_wrap_t(TextureWrap wrap)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_texture(0, *this);
    GLenum gl_target = get_internal_as<GL::GLTextureInternal>().m_target;
    glTexParameteri(gl_target, GL_TEXTURE_WRAP_T, GL::type(wrap));
    glCheck();
}

void Texture::set_wrap_r(TextureWrap wrap)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_texture(0, *this);
    GLenum gl_target = get_internal_as<GL::GLTextureInternal>().m_target;
    glTexParameteri(gl_target, GL_TEXTURE_WRAP_R, GL::type(wrap));
    glCheck();
}

void Texture::set_border_color(const std::array<float, 4>& color)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_texture(0, *this);
    GLenum gl_target = get_internal_as<GL::GLTextureInternal>().m_target;
    glTexParameterfv(gl_target, GL_TEXTURE_BORDER_COLOR, color.data());
    glCheck();
}

} // namespace Slic3r::App::Render
