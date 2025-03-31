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

    void init(Render::ImguiRender* imgui_render, std::function<void()> slice_fn , std::function<void()> export_fn) { 
        m_imgui_render = imgui_render; 
        m_slice_fn = slice_fn;
        m_export_fn = export_fn;
    }

    void render(Domain::Vec2f pos, Domain::Vec2f size);
    void set_export_allowed(bool allowed) { m_export_allowed = allowed; }
protected:

private:
    std::function<void()> m_slice_fn;
    Render::ImguiRender* m_imgui_render{ nullptr };

    bool m_export_allowed{ false };
    std::function<void()> m_export_fn;

    void on_slice();
};

} // namespace Slic3r::App::Plater
