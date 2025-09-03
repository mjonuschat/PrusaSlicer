#pragma once

#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Navigator.hpp"

#include <string>

namespace Slic3r::App::Render {
class ImguiRender;
}

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App {

class SidebarActionButtons : public Yoga::Window
{
public:
    SidebarActionButtons(const std::string& name, Render::ModuleType type, Navigator* render_module_navigator);

    virtual void on_init(Biz::ProjectInteractor* project_interactor) = 0;

protected:

    std::unique_ptr<Yoga::LayoutButton> get_navigation_button();
    Domain::SlicingId active_bed_slicing_id() const;
    void navigate_to_other();

protected:
    Biz::ProjectInteractor* m_project_interactor{nullptr};
    Navigator* m_render_module_navigator{nullptr};

    // The type of the owning render module.
    Render::ModuleType m_type{Render::ModuleType::Undef};
    // The type of the render module we need to navigate to.
    Render::ModuleType m_navigate_to_type{Render::ModuleType::Undef};

    std::string m_navigator_name;
    std::string m_navigator_tooltip;

    static constexpr ImColor color_primary{223, 93, 45};
    static constexpr ImColor color_secondary{ImVec4(0.32f, 0.48f, 0.84f, 1.0f)};
    static constexpr ImColor color_error{ImVec4(0.79f, 0.18f, 0.14f, 1.0f)};
    static constexpr float button_height{45};
    static constexpr float navig_btn_width{40.f};
};

} // namespace Slic3r::App
