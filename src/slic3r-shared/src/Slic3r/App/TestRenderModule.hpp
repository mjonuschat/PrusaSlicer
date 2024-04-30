#pragma once
#include <cstddef>
#include "slic3r-platform/src/Slic3r/App/Platform/AbstractRenderModule.hpp"

namespace Slic3r::App {

class TestRenderModule : public Platform::AbstractRenderModule
{
public:
    void render_scene() override;
    void render_imgui() override;

    void on_scene_mouse_event(const Platform::MouseEvent &e) override;
    void on_scene_keyboard_event(const Platform::KeyboardEvent &e) override;

private:
    static constexpr size_t BUF_SIZE = 256;
    char m_text_buffer[BUF_SIZE];
    bool m_main_window_opened{true};
};

} // namespace Slic3r::App::SDLTest
