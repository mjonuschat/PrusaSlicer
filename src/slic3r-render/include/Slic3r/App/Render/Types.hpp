#pragma once

#include <vector>

namespace Slic3r::App::Render {

class ScreenInfo;

struct RgbaF {
    float r;
    float g;
    float b;
    float a;
};

enum class PixelFormat
{
    RGB8 = 0,
    RGBA8,
};


enum class BufferTarget {
    VertexBuffer,
    IndexBuffer
};

enum class BufferUsage {
    StaticDraw,
    DynamicDraw,
    StreamDraw
};

struct Rect
{
    int x;
    int y;
    int width;
    int height;

    static Rect from(int x, int y, const ScreenInfo& screen);
};

enum class BlendFactor
{
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha
};

struct BlendOp
{
    BlendFactor src = BlendFactor::One;
    BlendFactor dst = BlendFactor::Zero;
};

enum class BlendEquation
{
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

struct Blending
{
    BlendOp rgb;
    BlendOp alpha;
    BlendEquation equation{BlendEquation::Add};
};

enum class PrimitiveType
{
    Points = 0,
    LineStrip,
    LineLoop,
    Lines,
    TriangleStrip,
    TriangleFan,
    Triangles
};

enum class ShaderType
{
    Vertex,
    Fragment,
    Geometry,
    TessEvaluation,
    TessControl,
    Compute,
    Count
};

/**
 * Vertex Attribute semantic as recognized by Shader
 */
enum class VertexAttribType
{
    Vertex = 0,
    Normal,
    TexCoord0,
    Color,
    Extra
};


/**
 * Data type of attribute representation in memory
 */
enum class DataType
{
    Float = 0,
    Byte,
    UByte,
    Short
};

enum class IndexType
{
    UByte = 0,
    UShort,
    UInt
};

template <typename I> struct IndexTypeTraits {};
template <> struct IndexTypeTraits<unsigned char> {
    static constexpr IndexType index_type = IndexType::UByte;
};
template <> struct IndexTypeTraits<unsigned short> {
    static constexpr IndexType index_type = IndexType::UShort;
};
template <> struct IndexTypeTraits<unsigned int> {
    static constexpr IndexType index_type = IndexType::UInt;
};

struct DrawCommand
{
    PrimitiveType primitive;
    size_t offset{0};
    size_t count{0};
};
using DrawCommands = std::vector<DrawCommand>;

}
