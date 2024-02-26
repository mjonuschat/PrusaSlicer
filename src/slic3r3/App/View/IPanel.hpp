#pragma once

namespace Slic3r::App::View {

struct IPanel
{
    virtual ~IPanel() = default;
    virtual void render_imgui() = 0;
};

} // namespace Slic3r::App::View
