#pragma once
#include <SDL.h>
#include <imgui/imgui.h>

#include <Slic3r/App/Platform/AbstractRenderCanvas.hpp>

namespace Slic3r::App::Platform::SDL {

class SDLRenderCanvas : public AbstractRenderCanvas
{
public:
    SDLRenderCanvas();
    ~SDLRenderCanvas() override;

    void poll_events();
    bool should_quit() const { return m_should_quit; }

protected:
    void begin_frame_platform() override;
    void begin_imgui_frame_platform() override;
    void end_imgui_frame_platform() override;
    void end_frame_platform() override;
    double get_platform_time() override;

private:
    void init_sdl_imgui();

    void update_imgui_mouse_position();
    void update_imgui_mouse_cursor();
    bool pass_event_to_imgui(SDL_Event& event);

    void pass_event_to_scene(SDL_Event& event);

private:
    SDL_Window* m_window{nullptr};
    SDL_Cursor* m_mouse_cursors[ImGuiMouseCursor_COUNT] {};
    char* m_clipboard_text_data{nullptr};
    SDL_GLContext m_gl_context{nullptr};
    bool m_mouse_button_pressed[3] {false, false, false};
    bool m_mouse_can_use_global_state{true};
    bool m_should_quit{false};
};

}
