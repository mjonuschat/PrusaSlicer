#pragma once
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/StatusCache.hpp"
#include "Slic3r/Biz/FDMResultCache.hpp"
#include "Slic3r/App/IRenderModuleChangedListener.hpp"

#include <string>

namespace Slic3r::App::Render {
class ImguiRender;
}

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App {

class SidebarActionButtons : public WithListeners<IRenderModuleChangedListener>
{
public:
    SidebarActionButtons() {}

    void on_init(
        Biz::ProjectInteractor* project_interactor,
        Render::ImguiRender*    imgui_render,
        Render::ModuleType      type);

    void render(Domain::Vec2f pos, Domain::Vec2f size);

private:
    bool slice_allowed() const;
    bool export_allowed() const;
    void navigate_to_other();

    void render_export_buttons();
    void render_navigation_button();
    void render_slice_button(Domain::Vec2f size);

private:
    Render::ImguiRender*        m_imgui_render          { nullptr };
    Biz::ProjectInteractor*     m_project_interactor    { nullptr };

    // The type of the owning render module.
    Render::ModuleType          m_type                  { Render::ModuleType::Undef };
    // The type of the render module we need to navigate to.
    Render::ModuleType          m_navigate_to_type      { Render::ModuleType::Undef };

    std::string m_navigator_name        {};
    std::string m_navigator_tooltip     {};
};

} // namespace Slic3r::App
