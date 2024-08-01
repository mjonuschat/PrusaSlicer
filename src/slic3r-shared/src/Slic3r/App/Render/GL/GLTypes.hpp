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
        // Short
        GL_SHORT
    };

    const size_t idx = static_cast<size_t>(type);
    ASSERT(0 >= idx && idx < (sizeof(translation_table) / sizeof(translation_table[0])));

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
}

inline GLenum type(BufferUsage usage)
{
    switch (usage) {
    case BufferUsage::StaticDraw:
        return GL_STATIC_DRAW;
    }
}



const char* shader_input_name(VertexAttribType vat);
GLenum texture_format(PixelFormat format);
GLenum texture_format_type(PixelFormat format);
GLenum type(BlendFactor type);

}
