#pragma once

#include "WithInternal.hpp"
#include "Types.hpp"

namespace Slic3r::App::Render {

class Renderbuffer : public WithInternal
{
public:
    Renderbuffer(PixelFormat format);
    ~Renderbuffer() override;

    PixelFormat format() const { return m_format; }

protected:
    PixelFormat m_format;
};

using RenderbufferPtr = std::unique_ptr<Renderbuffer>;
using RenderbufferPtrs = std::vector<RenderbufferPtr>;

} // namespace Slic3r::App::Render
