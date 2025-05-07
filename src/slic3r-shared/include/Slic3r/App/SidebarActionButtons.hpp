#pragma once

#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Biz/Slicing/BackgroundProcess.hpp"
#include "Slic3r/App/IRenderModuleChangedListener.hpp"
#include "Slic3r/App/Yoga/Window.hpp"

#include <string>

namespace Slic3r::App::Render {
class ImguiRender;
}

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App {

class SidebarActionButtons : public Yoga::Window, public WithListeners<IRenderModuleChangedListener>
{
public:
    explicit SidebarActionButtons(
        const std::string& name, Render::ModuleType type, Yoga::Item* parent = nullptr
    );

    void on_init(Biz::ProjectInteractor* project_interactor);

protected:
    Biz::Slicing::SlicingId active_bed_slicing_id() const;
    bool slice_allowed() const;
    void navigate_to_other();

protected:
    Biz::ProjectInteractor* m_project_interactor{nullptr};

    // The type of the owning render module.
    Render::ModuleType m_type{Render::ModuleType::Undef};
    // The type of the render module we need to navigate to.
    Render::ModuleType m_navigate_to_type{Render::ModuleType::Undef};

    std::string m_navigator_name;
    std::string m_navigator_tooltip;

    static constexpr ImColor color_primary{223, 93, 45};
    static constexpr ImColor color_secondary{ImVec4(0.32f, 0.48f, 0.84f, 1.0f)};
    static constexpr float button_height{45};
};

} // namespace Slic3r::App
