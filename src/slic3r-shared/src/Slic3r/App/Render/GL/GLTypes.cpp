#include "GLTypes.hpp"

#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Render::GL {

const char* shader_input_name(VertexAttribType vat)
{
    switch (vat) {
    case VertexAttribType::Vertex:
        return "v_position";

    case VertexAttribType::Normal:
        return "v_normal";

    case VertexAttribType::TexCoord0:
        return "v_tex_coord";

    case VertexAttribType::Color:
        return "v_color";

    case VertexAttribType::Extra:
        return "v_extra";

    }

    // Encountered missing VertexAttribType, if valid, please add it into the switch above
    ASSERT(false);
    return "";
}


GLenum texture_format(PixelFormat format)
{
    switch (format) {
    case PixelFormat::RGB8:
        return GL_RGB;

    case PixelFormat::RGBA8:
        return GL_RGBA;

    default:
        // Unsupported format
        ASSERT(false);
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
        ASSERT(false);
        return GL_UNSIGNED_BYTE;
    }
}

GLenum type(BlendFactor type)
{
    constexpr static GLenum translation_table[] = {
        GL_ZERO,
        GL_ONE,
        GL_SRC_COLOR,
        GL_ONE_MINUS_SRC_COLOR,
        GL_DST_COLOR,
        GL_ONE_MINUS_DST_COLOR,
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA,
        GL_DST_ALPHA,
        GL_ONE_MINUS_DST_ALPHA
    };

    const int idx = static_cast<int>(type);
    ASSERT(idx >= 0 && idx < sizeof(translation_table)/sizeof(translation_table[0]));
    return translation_table[idx];
}

GLenum type(BlendEquation type)
{
    constexpr static GLenum translation_table[] = {
        GL_FUNC_ADD,
        GL_FUNC_SUBTRACT,
        GL_FUNC_REVERSE_SUBTRACT,
        GL_MIN,
        GL_MAX
    };

    const int idx = static_cast<int>(type);
    ASSERT(idx >= 0 && idx < sizeof(translation_table)/sizeof(translation_table[0]));
    return translation_table[idx];
}


}
