#pragma once

#include <vector>

namespace Slic3r::App::Render {
/**
 * Vertex Attribute semantic as recognized by Shader
 */
enum class VertexAttribType
{
    Vertex = 0,
    Normal,
    TexCoord0,
    Extra
};


/**
 * Data type of attribute representation in memory
 */
enum class DataType
{
    Float = 0,
    Byte,
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

/**
 * Description of a single vertex attribute stored in VertexBuffer.
 */
struct VertexAttribDesc
{
    /**
     * Attribute semantic
     */
    VertexAttribType attrib_type;

    /**
     * Attribute data type
     */
    DataType data_type;

    /**
     * Number of components
     */
    uint8_t components;

    /**
     * Offset from start in bytes
     */
    size_t offset;

    /**
     * Stride between vertices in bytes, pass 0 for tightly packed vertex layout.
     */
    //size_t stride;

    /**
     * All components size in bytes for given vertex attribute.
     * @return Vertex attribute size in bytes.
     */
    size_t size_in_bytes() const;
};

using VertexAttribsDesc = std::vector<VertexAttribDesc>;
size_t vertex_attribs_stride(const VertexAttribsDesc& attrs);
size_t index_type_size(IndexType index);

}