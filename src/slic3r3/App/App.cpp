#include "App.hpp"

#include <memory>
#include "slic3r3/Domain/Workbench.hpp"
#include "slic3r3/Domain/Bed.hpp"
#include "libslic3r/Model.hpp"
#include "View/TestRenderModule.hpp"
#include "slic3r3/App/Platform/IRenderingPlatform.hpp"

namespace Slic3r::App {

void App::init(int argc, char **argv) {
    m_workbench = std::make_unique<Domain::Workbench>();
    m_render_module = make_unique<View::TestRenderModule>();
}

void App::render(Platform::IRenderingPlatform& rendering_platform) {
    if (m_render_module)
    {
        rendering_platform.begin_frame();
        rendering_platform.begin_imgui_frame();
        m_render_module->render_imgui();
        rendering_platform.end_imgui_frame();
        m_render_module->render_scene();
        rendering_platform.end_frame();
    }
}


} // namespace Slic3r::App
