#pragma once

#include "commonGL.hpp"
#include "Slic3r/App/Render/Types.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Render::GL {
inline GLenum type(DataType type)
{
    constexpr static GLenum translation_table[] = {
        // Float = 0,
        GL_FLOAT,
        // Byte,
        GL_BYTE,
        // UByte
        GL_UNSIGNED_BYTE,
        // Short
        GL_SHORT
    };

    const size_t idx = static_cast<size_t>(type);
    ASSERT(idx < (sizeof(translation_table) / sizeof(translation_table[0])));

    return translation_table[idx];
}

inline GLenum type(IndexType type)
{
    constexpr static GLenum translation_table[] = {
        // UByte = 0,
        GL_UNSIGNED_BYTE,
        // UShort,
        GL_UNSIGNED_SHORT,
        // UInt
        GL_UNSIGNED_INT,
    };

    const int idx = static_cast<int>(type);
    ASSERT(idx >= 0 && idx < sizeof(translation_table) / sizeof(translation_table[0]));

    return translation_table[idx];
}

inline GLenum type(PrimitiveType type)
{
    constexpr static GLenum translation_table[] = {
        GL_POINTS,
        GL_LINE_STRIP,
        GL_LINE_LOOP,
        GL_LINES,
        GL_TRIANGLE_STRIP,
        GL_TRIANGLE_FAN,
        GL_TRIANGLES
    };

    const int idx = static_cast<int>(type);
    ASSERT(idx >= 0 && idx < sizeof(translation_table) / sizeof(translation_table[0]));

    return translation_table[idx];
}

inline GLenum type(BufferTarget target)
{
    switch (target)
    {
    case BufferTarget::VertexBuffer:
        return GL_ARRAY_BUFFER;
    case BufferTarget::IndexBuffer:
        return GL_ELEMENT_ARRAY_BUFFER;
    }
    throw std::runtime_error{"Unreachable code!"};
}

inline GLenum type(BufferUsage usage)
{
    switch (usage) {
    case BufferUsage::StaticDraw:
        return GL_STATIC_DRAW;
    case BufferUsage::DynamicDraw:
        return GL_DYNAMIC_DRAW;
    case BufferUsage::StreamDraw:
        return GL_STREAM_DRAW;
    }
    throw std::runtime_error{"Unreachable code!"};
}

inline GLenum type(TextureMinFilter filter)
{
    switch (filter)
    {
    case TextureMinFilter::Linear:               return GL_LINEAR;
    case TextureMinFilter::Nearest:              return GL_NEAREST;
    case TextureMinFilter::MipMapNearestNearest: return GL_NEAREST_MIPMAP_NEAREST;
    case TextureMinFilter::MipMapLinearNearest:  return GL_LINEAR_MIPMAP_NEAREST;
    case TextureMinFilter::MipMapNearestLinear:  return GL_NEAREST_MIPMAP_LINEAR;
    case TextureMinFilter::MipMapLinearLinear:   return GL_LINEAR_MIPMAP_LINEAR;
    }
    throw std::runtime_error{"Unreachable code!"};
}

inline GLenum type(TextureMagFilter filter)
{
    switch (filter)
    {
    case TextureMagFilter::Linear:  return GL_LINEAR;
    case TextureMagFilter::Nearest: return GL_NEAREST;
    }
    throw std::runtime_error{"Unreachable code!"};
}


const char* shader_input_name(VertexAttribType vat);
GLenum texture_internal_format(PixelFormat format);
GLenum texture_format(PixelFormat format);
GLenum texture_format_type(PixelFormat format);
GLenum type(BlendFactor type);
GLenum type(BlendEquation type);


}
