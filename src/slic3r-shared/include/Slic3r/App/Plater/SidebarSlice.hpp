#pragma once
#include "Slic3r/Domain/Types.hpp"
#include <functional>

namespace Slic3r::App::Render {
class ImguiRender;
}

namespace Slic3r::App::Plater {

class SidebarSlice
{
public:
    SidebarSlice() {}

    void init(Render::ImguiRender* imgui_render, std::function<void()> slice_fn) { 
        m_imgui_render = imgui_render; 
        m_slice_fn = slice_fn;
    }

    void render(Domain::Vec2f pos, Domain::Vec2f size);

protected:

private:
    std::function<void()> m_slice_fn;
    Render::ImguiRender* m_imgui_render{ nullptr };
};

} // namespace Slic3r::App::Plater
