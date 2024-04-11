//
// Created by Jan Bartipan on 21.03.2024.
//

#pragma once

#include <cassert>
#include <GL/glew.h>

namespace Slic3r::App::Renderer {

enum class BufferType: uint8_t
{
    Vertex = 0, Index
};

GLenum buffer_target(BufferType type) {
    switch (type) {
    case BufferType::Vertex:
        return GL_ARRAY_BUFFER;
    case BufferType::Index:
        return GL_ELEMENT_ARRAY_BUFFER;
    }
    assert(false);
}


class Device {
public:
    using ResourceHandle = GLuint;

#ifdef DEBUG
#define DEVICE_CHECK_ERROR() check_errors()
    void check_errors()
    {
        GLenum error = glGetError();
        assert(error == GL_NO_ERROR);
    }
#else
#define DEVICE_CHECK_ERROR()
#endif


    ResourceHandle create_buffer()
    {
        ResourceHandle ret;
        glCreateBuffers(1, &ret);
        DEVICE_CHECK_ERROR();
        return ret;
    }

    void dispose_buffer(ResourceHandle id)
    {
        glDeleteBuffers(1, &id);
        DEVICE_CHECK_ERROR();
    }

    void bind_buffer(ResourceHandle id, BufferType type)
    {
        glBindBuffer(buffer_target(type), id);
        DEVICE_CHECK_ERROR();
    }

    void set_buffer_data(ResourceHandle id, BufferType type, const void* data, size_t size)
    {
        glBufferData(buffer_target(type), size, data,  GL_STREAM_DRAW);
        DEVICE_CHECK_ERROR();
    }

    ResourceHandle create_texture()
    {
        ResourceHandle ret;
        glGenTextures(1, &ret);
        DEVICE_CHECK_ERROR();
        return ret;
    }

    void dispose_texture(ResourceHandle id)
    {
        glDeleteTextures(1, &id);
        DEVICE_CHECK_ERROR();
    }

    void set_texture_data(ResourceHandle id, void* data)
    {
    }


#undef DEVICE_CHECK_ERROR

};


}

