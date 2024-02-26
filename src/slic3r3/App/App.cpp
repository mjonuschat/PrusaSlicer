#include "App.hpp"

#include <memory>

#include "View/TestFrame.hpp"


namespace Slic3r::App {

void App::init(int argc, char **argv) {
    m_main_frame = std::make_unique<View::TestFrame>();
    m_active_frame = m_main_frame.get();
}

void App::render_imgui() {
    if (m_active_frame)
        m_active_frame->render_imgui();
}

void App::render() {
    if (m_active_frame)
        m_active_frame->render_background();
}

void App::navigate_to_platter() {}
void App::navigate_to_config() {}

} // namespace Slic3r::App
