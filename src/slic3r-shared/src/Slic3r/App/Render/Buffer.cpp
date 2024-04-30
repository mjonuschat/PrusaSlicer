#include "Buffer.hpp"

namespace Slic3r::App::Render {

Buffer::~Buffer()
{
    if (m_id)
        glDeleteBuffers(1, &m_id);
}


} // namespace Slic3r::App::Render
