#include "SDLRenderCanvas.hpp"

#include <string>

#include <gl/glew.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <Slic3r/App/Platform/PlatformError.hpp>
#include <Slic3r/App/Render/commonGL.hpp>

namespace Slic3r::App::Platform::SDL
{

SDLRenderCanvas::SDLRenderCanvas()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::string message = std::string("Platform Error: ") + SDL_GetError();
        throw PlatformError(message);
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create m_window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    m_window = SDL_CreateWindow("Slic3r3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    m_gl_context = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_gl_context);
#ifndef __EMSCRIPTEN__
    SDL_GL_SetSwapInterval(1); // Enable vsync
#endif

    const auto err = glewInit();
    if (err != GLEW_NO_ERROR) {
        throw PlatformError(std::string("GLEW init failed with code ") + std::to_string(err));
    }

    Render::initialize_gl();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
#endif

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    init_sdl_imgui();
    ImGui_ImplOpenGL3_Init(glsl_version);
}

SDLRenderCanvas::~SDLRenderCanvas()
{
    std::cerr << "SDLRenderCanvas dtor\n";

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();

    // Destroy last known clipboard data
    if (m_clipboard_text_data)
        SDL_free(m_clipboard_text_data);
    m_clipboard_text_data = NULL;

    // Destroy SDL mouse cursors
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        SDL_FreeCursor(m_mouse_cursors[cursor_n]);
    memset(m_mouse_cursors, 0, sizeof(m_mouse_cursors));

    ImGui::DestroyContext();

    SDL_GL_DeleteContext(m_gl_context);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void SDLRenderCanvas::init_sdl_imgui()
{
    // Setup backend capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;       // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "imgui_impl_sdl";

    // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
    io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

//    io.SetClipboardTextFn = ImGui_ImplSDL2_SetClipboardText;
//    io.GetClipboardTextFn = ImGui_ImplSDL2_GetClipboardText;
//    io.ClipboardUserData = NULL;

    // Load mouse cursors
    m_mouse_cursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    m_mouse_cursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    m_mouse_cursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    m_mouse_cursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    m_mouse_cursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    m_mouse_cursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    m_mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    m_mouse_cursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    m_mouse_cursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

    // Check and store if we are on Wayland
    m_mouse_can_use_global_state = strncmp(SDL_GetCurrentVideoDriver(), "wayland", 7) != 0;

#ifdef _WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(m_window, &wmInfo);
    io.ImeWindowHandle = wmInfo.info.win.window;
#endif


}


void SDLRenderCanvas::poll_events()
{
    // Poll and handle events (inputs, m_window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        process_event(event);
    }
}

void SDLRenderCanvas::wait_for_events()
{
    SDL_Event event;
    bool render_required = false;
    while (!render_required) {
        if (SDL_WaitEventTimeout(&event, 1000 / 60) == 1) {
            process_event(event);
            render_required = true;
        }
        render_required |= get_and_reset_render_requested();
        render_required |= AbstractRenderCanvas::dispatch_enqueued();
    }
}

void SDLRenderCanvas::process_event(const SDL_Event& event)
{
    pass_event_to_imgui(event);
    pass_event_to_scene(event);

    if (event.type == SDL_QUIT)
        m_should_quit = true;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_window))
        m_should_quit = true;

}

bool SDLRenderCanvas::pass_event_to_imgui(const SDL_Event& event)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (event.type)
    {
    case SDL_MOUSEWHEEL:
    {
        if (event.wheel.x > 0) io.MouseWheelH += 1;
        if (event.wheel.x < 0) io.MouseWheelH -= 1;
        if (event.wheel.y > 0) io.MouseWheel += 1;
        if (event.wheel.y < 0) io.MouseWheel -= 1;
        return true;
    }
    case SDL_MOUSEBUTTONDOWN:
    {
        if (event.button.button == SDL_BUTTON_LEFT) m_mouse_button_pressed[0] = true;
        if (event.button.button == SDL_BUTTON_RIGHT) m_mouse_button_pressed[1] = true;
        if (event.button.button == SDL_BUTTON_MIDDLE) m_mouse_button_pressed[2] = true;
        return true;
    }
    case SDL_TEXTINPUT:
    {
        io.AddInputCharactersUTF8(event.text.text);
        return true;
    }
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        int key = event.key.keysym.scancode;
        assert(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
        io.KeysDown[key] = (event.type == SDL_KEYDOWN);
        io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
        io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
        io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
#ifdef _WIN32
        io.KeySuper = false;
#else
        io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
#endif
        return true;
    }
    }
    return false;
}

void SDLRenderCanvas::begin_frame_platform()
{
    ImGuiIO& io = ImGui::GetIO();
    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(m_window, &w, &h);
    if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED)
        w = h = 0;
    SDL_GL_GetDrawableSize(m_window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    set_screen_size(display_w, display_h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
}

void SDLRenderCanvas::begin_imgui_frame_platform()
{
    update_imgui_mouse_position();
    update_imgui_mouse_cursor();
}

void SDLRenderCanvas::end_imgui_frame_platform()
{}

void SDLRenderCanvas::end_frame_platform()
{
    SDL_GL_SwapWindow(m_window);
}

void SDLRenderCanvas::update_imgui_mouse_position()
{
    ImGuiIO& io = ImGui::GetIO();

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos)
        SDL_WarpMouseInWindow(m_window, (int)io.MousePos.x, (int)io.MousePos.y);
    else
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

    int mx, my;
    Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);
    io.MouseDown[0] = m_mouse_button_pressed[0] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
    io.MouseDown[1] = m_mouse_button_pressed[1] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    io.MouseDown[2] = m_mouse_button_pressed[2] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    m_mouse_button_pressed[0] = m_mouse_button_pressed[1] = m_mouse_button_pressed[2] = false;

#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && !(defined(__APPLE__) && TARGET_OS_IOS)
    SDL_Window* focused_window = SDL_GetKeyboardFocus();
    if (m_window == focused_window)
    {
        if (m_mouse_can_use_global_state)
        {
            // SDL_GetMouseState() gives mouse position seemingly based on the last window entered/focused(?)
            // The creation of a new windows at runtime and SDL_CaptureMouse both seems to severely mess up with that, so we retrieve that position globally.
            // Won't use this workaround when on Wayland, as there is no global mouse position.
            int wx, wy;
            SDL_GetWindowPosition(focused_window, &wx, &wy);
            SDL_GetGlobalMouseState(&mx, &my);
            mx -= wx;
            my -= wy;
        }
        io.MousePos = ImVec2((float)mx, (float)my);
    }

    // SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the SDL window boundaries shouldn't e.g. trigger the OS window resize cursor.
    // The function is only supported from SDL 2.0.4 (released Jan 2016)
    bool any_mouse_button_down = ImGui::IsAnyMouseDown();
    SDL_CaptureMouse(any_mouse_button_down ? SDL_TRUE : SDL_FALSE);
#else
    if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS)
        io.MousePos = ImVec2((float)mx, (float)my);
#endif

}

void SDLRenderCanvas::update_imgui_mouse_cursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_ShowCursor(SDL_FALSE);
    }
    else
    {
        // Show OS mouse cursor
        SDL_SetCursor(m_mouse_cursors[imgui_cursor] ? m_mouse_cursors[imgui_cursor] : m_mouse_cursors[ImGuiMouseCursor_Arrow]);
        SDL_ShowCursor(SDL_TRUE);
    }
}


double SDLRenderCanvas::get_platform_time()
{
    static Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 current_time = SDL_GetPerformanceCounter();
    return double(current_time) / frequency;
}

void SDLRenderCanvas::pass_event_to_scene(const SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
    {
        const auto &key = event.key;
        const KeyboardEvent::Type type = key.state == SDL_PRESSED ? KeyboardEvent::Type::KeyDown :
                                                                    KeyboardEvent::Type::KeyUp;
        // Platform independent keycode is mapped 1:1 to SDL KeyCode
        const auto code = KeyCode(key.keysym.sym);
        update_key_modifiers(type, code);

        KeyboardEvent platform_event{type, code, m_key_modifiers};
        enqueue_keyboard(platform_event);
    }
    else if (event.type == SDL_MOUSEMOTION)
    {
        const auto &mouse = event.motion;
        update_mouse_position(mouse.x, mouse.y);
        MouseEvent platform_event{
            MouseEvent::Type::Move, MouseButton::None,
            mouse.x, mouse.y,
            0, 0,
            m_key_modifiers
        };
        enqueue_mouse(platform_event);
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
    {
        const auto& mouse_button = event.button;
        MouseEvent::Type type = event.type == SDL_MOUSEBUTTONDOWN ? MouseEvent::Type::ButtonDown : MouseEvent::Type::ButtonUp;
        MouseEvent platform_event{
            type, MouseButton(mouse_button.button),
            m_mouse_x, m_mouse_y,
            0, 0,
            m_key_modifiers
        };
        enqueue_mouse(platform_event);
    }
    else if (event.type == SDL_MOUSEWHEEL)
    {
        const auto& mouse_wheel = event.wheel;
        update_mouse_position(mouse_wheel.mouseX, mouse_wheel.mouseY);
        MouseEvent platform_event{
            MouseEvent::Type::Wheel, MouseButton::None,
            m_mouse_x, m_mouse_y,
            mouse_wheel.preciseX, mouse_wheel.preciseY,
            m_key_modifiers
        };
        enqueue_mouse(platform_event);
    }
}

}