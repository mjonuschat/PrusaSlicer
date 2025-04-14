#pragma once

#include "WithInternal.hpp"
#include "Types.hpp"

namespace Slic3r::App::Render {

class Device;
class Texture;

struct FramebufferColorAttachment
{
    PixelFormat format{ PixelFormat::RGBA8 };
    TextureMinFilter min_filter{ TextureMinFilter::Nearest };
    TextureMagFilter mag_filter{ TextureMagFilter::Nearest };
};

struct FramebufferCreationData
{
    FramebufferTarget target{ FramebufferTarget::Framebuffer };
    size_t width{ 0 };
    size_t height{ 0 };
    std::vector<FramebufferColorAttachment> color_attachments;
    bool depth{ true };
    bool stencil{ false };
};

class Framebuffer : public WithInternal
{
public:
    Framebuffer(Device& device, const FramebufferCreationData& data);
    ~Framebuffer() override;

    FramebufferTarget target() const { return m_target; }
    const Texture* color_attachment(size_t id) const;
    const Texture* depth() const;
    const Texture* stencil() const;

private:
    Device& m_device;
    FramebufferTarget m_target;
    std::vector<std::unique_ptr<Texture>> m_textures;
};

} // namespace Slic3r::App::Render
