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


void Texture::set_data(PixelFormat format,size_t level, size_t w, size_t h, const void* data)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_texture(0, *this);

    GLenum gl_format = GL::texture_format(format);
    GLenum gl_type = GL::texture_format_type(format);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, level > 0 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
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
