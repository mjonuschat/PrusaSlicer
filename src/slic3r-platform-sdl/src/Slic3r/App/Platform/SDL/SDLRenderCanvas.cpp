#include <string>

#include <GL/glew.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <Slic3r/App/Platform/PlatformError.hpp>
#include <Slic3r/App/Render/Init.hpp>
#include <Slic3r/App/Render/Context.hpp>
#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Render/Geometry.hpp>
#include <Slic3r/App/Render/Texture.hpp>
#include <Slic3r/App/Platform/MouseEvent.hpp>

// Eigen headers clash with SDL headers, because of macro Success.
// Include SDL headers last.
#include "Slic3r/App/Platform/SDL/SDLRenderCanvas.hpp"
#include "SDL_syswm.h"

namespace Slic3r::App::Platform::SDL
{

SDLRenderCanvas::SDLRenderCanvas(std::unique_ptr<StdMainThreadDispatcher>&& main_thread_dispatcher)
    : AbstractRenderCanvas{std::move(main_thread_dispatcher)}
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
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

    Render::initialize_render();

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
}

SDLRenderCanvas::~SDLRenderCanvas()
{
    // Destroy last known clipboard data
    if (m_clipboard_text_data)
        SDL_free(m_clipboard_text_data);
    m_clipboard_text_data = NULL;

    // Destroy SDL mouse cursors
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        SDL_FreeCursor(m_mouse_cursors[cursor_n]);
    memset(m_mouse_cursors, 0, sizeof(m_mouse_cursors));

    ImGui::DestroyContext();
    Render::shutdown_render();

    SDL_GL_DeleteContext(m_gl_context);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

// see: imgui_impl_sdl2.cpp line 184
static ImGuiKey sdl2_to_imgui_key(SDL_Keycode keycode)
{
    switch (keycode)
    {
        case SDLK_TAB:          return ImGuiKey_Tab;
        case SDLK_LEFT:         return ImGuiKey_LeftArrow;
        case SDLK_RIGHT:        return ImGuiKey_RightArrow;
        case SDLK_UP:           return ImGuiKey_UpArrow;
        case SDLK_DOWN:         return ImGuiKey_DownArrow;
        case SDLK_PAGEUP:       return ImGuiKey_PageUp;
        case SDLK_PAGEDOWN:     return ImGuiKey_PageDown;
        case SDLK_HOME:         return ImGuiKey_Home;
        case SDLK_END:          return ImGuiKey_End;
        case SDLK_INSERT:       return ImGuiKey_Insert;
        case SDLK_DELETE:       return ImGuiKey_Delete;
        case SDLK_BACKSPACE:    return ImGuiKey_Backspace;
        case SDLK_SPACE:        return ImGuiKey_Space;
        case SDLK_RETURN:       return ImGuiKey_Enter;
        case SDLK_ESCAPE:       return ImGuiKey_Escape;
        case SDLK_QUOTE:        return ImGuiKey_Apostrophe;
        case SDLK_COMMA:        return ImGuiKey_Comma;
        case SDLK_MINUS:        return ImGuiKey_Minus;
        case SDLK_PERIOD:       return ImGuiKey_Period;
        case SDLK_SLASH:        return ImGuiKey_Slash;
        case SDLK_SEMICOLON:    return ImGuiKey_Semicolon;
        case SDLK_EQUALS:       return ImGuiKey_Equal;
        case SDLK_LEFTBRACKET:  return ImGuiKey_LeftBracket;
        case SDLK_BACKSLASH:    return ImGuiKey_Backslash;
        case SDLK_RIGHTBRACKET: return ImGuiKey_RightBracket;
        case SDLK_BACKQUOTE:    return ImGuiKey_GraveAccent;
        case SDLK_CAPSLOCK:     return ImGuiKey_CapsLock;
        case SDLK_SCROLLLOCK:   return ImGuiKey_ScrollLock;
        case SDLK_NUMLOCKCLEAR: return ImGuiKey_NumLock;
        case SDLK_PRINTSCREEN:  return ImGuiKey_PrintScreen;
        case SDLK_PAUSE:        return ImGuiKey_Pause;
        case SDLK_KP_0:         return ImGuiKey_Keypad0;
        case SDLK_KP_1:         return ImGuiKey_Keypad1;
        case SDLK_KP_2:         return ImGuiKey_Keypad2;
        case SDLK_KP_3:         return ImGuiKey_Keypad3;
        case SDLK_KP_4:         return ImGuiKey_Keypad4;
        case SDLK_KP_5:         return ImGuiKey_Keypad5;
        case SDLK_KP_6:         return ImGuiKey_Keypad6;
        case SDLK_KP_7:         return ImGuiKey_Keypad7;
        case SDLK_KP_8:         return ImGuiKey_Keypad8;
        case SDLK_KP_9:         return ImGuiKey_Keypad9;
        case SDLK_KP_PERIOD:    return ImGuiKey_KeypadDecimal;
        case SDLK_KP_DIVIDE:    return ImGuiKey_KeypadDivide;
        case SDLK_KP_MULTIPLY:  return ImGuiKey_KeypadMultiply;
        case SDLK_KP_MINUS:     return ImGuiKey_KeypadSubtract;
        case SDLK_KP_PLUS:      return ImGuiKey_KeypadAdd;
        case SDLK_KP_ENTER:     return ImGuiKey_KeypadEnter;
        case SDLK_KP_EQUALS:    return ImGuiKey_KeypadEqual;
        case SDLK_LCTRL:        return ImGuiKey_LeftCtrl;
        case SDLK_LSHIFT:       return ImGuiKey_LeftShift;
        case SDLK_LALT:         return ImGuiKey_LeftAlt;
        case SDLK_LGUI:         return ImGuiKey_LeftSuper;
        case SDLK_RCTRL:        return ImGuiKey_RightCtrl;
        case SDLK_RSHIFT:       return ImGuiKey_RightShift;
        case SDLK_RALT:         return ImGuiKey_RightAlt;
        case SDLK_RGUI:         return ImGuiKey_RightSuper;
        case SDLK_APPLICATION:  return ImGuiKey_Menu;
        case SDLK_0:            return ImGuiKey_0;
        case SDLK_1:            return ImGuiKey_1;
        case SDLK_2:            return ImGuiKey_2;
        case SDLK_3:            return ImGuiKey_3;
        case SDLK_4:            return ImGuiKey_4;
        case SDLK_5:            return ImGuiKey_5;
        case SDLK_6:            return ImGuiKey_6;
        case SDLK_7:            return ImGuiKey_7;
        case SDLK_8:            return ImGuiKey_8;
        case SDLK_9:            return ImGuiKey_9;
        case SDLK_a:            return ImGuiKey_A;
        case SDLK_b:            return ImGuiKey_B;
        case SDLK_c:            return ImGuiKey_C;
        case SDLK_d:            return ImGuiKey_D;
        case SDLK_e:            return ImGuiKey_E;
        case SDLK_f:            return ImGuiKey_F;
        case SDLK_g:            return ImGuiKey_G;
        case SDLK_h:            return ImGuiKey_H;
        case SDLK_i:            return ImGuiKey_I;
        case SDLK_j:            return ImGuiKey_J;
        case SDLK_k:            return ImGuiKey_K;
        case SDLK_l:            return ImGuiKey_L;
        case SDLK_m:            return ImGuiKey_M;
        case SDLK_n:            return ImGuiKey_N;
        case SDLK_o:            return ImGuiKey_O;
        case SDLK_p:            return ImGuiKey_P;
        case SDLK_q:            return ImGuiKey_Q;
        case SDLK_r:            return ImGuiKey_R;
        case SDLK_s:            return ImGuiKey_S;
        case SDLK_t:            return ImGuiKey_T;
        case SDLK_u:            return ImGuiKey_U;
        case SDLK_v:            return ImGuiKey_V;
        case SDLK_w:            return ImGuiKey_W;
        case SDLK_x:            return ImGuiKey_X;
        case SDLK_y:            return ImGuiKey_Y;
        case SDLK_z:            return ImGuiKey_Z;
        case SDLK_F1:           return ImGuiKey_F1;
        case SDLK_F2:           return ImGuiKey_F2;
        case SDLK_F3:           return ImGuiKey_F3;
        case SDLK_F4:           return ImGuiKey_F4;
        case SDLK_F5:           return ImGuiKey_F5;
        case SDLK_F6:           return ImGuiKey_F6;
        case SDLK_F7:           return ImGuiKey_F7;
        case SDLK_F8:           return ImGuiKey_F8;
        case SDLK_F9:           return ImGuiKey_F9;
        case SDLK_F10:          return ImGuiKey_F10;
        case SDLK_F11:          return ImGuiKey_F11;
        case SDLK_F12:          return ImGuiKey_F12;
        case SDLK_F13:          return ImGuiKey_F13;
        case SDLK_F14:          return ImGuiKey_F14;
        case SDLK_F15:          return ImGuiKey_F15;
        case SDLK_F16:          return ImGuiKey_F16;
        case SDLK_F17:          return ImGuiKey_F17;
        case SDLK_F18:          return ImGuiKey_F18;
        case SDLK_F19:          return ImGuiKey_F19;
        case SDLK_F20:          return ImGuiKey_F20;
        case SDLK_F21:          return ImGuiKey_F21;
        case SDLK_F22:          return ImGuiKey_F22;
        case SDLK_F23:          return ImGuiKey_F23;
        case SDLK_F24:          return ImGuiKey_F24;
        case SDLK_AC_BACK:      return ImGuiKey_AppBack;
        case SDLK_AC_FORWARD:   return ImGuiKey_AppForward;
        default:                break;
    }
    return ImGuiKey_None;
}

void SDLRenderCanvas::init_sdl_imgui()
{
    // Setup backend capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;       // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "imgui_impl_sdl";

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

    // see: imgui_impl_sdl2.cpp line 499 
    // Set platform dependent data in viewport
    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)(intptr_t)SDL_GetWindowID(m_window);
    main_viewport->PlatformHandleRaw = nullptr;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(m_window, &info)) {
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
        main_viewport->PlatformHandleRaw = (void*)info.info.win.window;
#elif defined(__APPLE__) && defined(SDL_VIDEO_DRIVER_COCOA)
        main_viewport->PlatformHandleRaw = (void*)info.info.cocoa.window;
#endif // SDL_VIDEO_DRIVER_WINDOWS
    }
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
        render_required |= m_main_thread_dispatcher->dispatch_enqueued();
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
        ImGuiKey imgui_key = sdl2_to_imgui_key(event.key.keysym.sym);
        if (imgui_key != ImGuiKey_None)
            io.AddKeyEvent(imgui_key, event.type == SDL_KEYDOWN);
        SDL_Keymod sdl_key_mods = (SDL_Keymod)event.key.keysym.mod;
        io.AddKeyEvent(ImGuiMod_Ctrl,  (sdl_key_mods & KMOD_CTRL) != 0);
        io.AddKeyEvent(ImGuiMod_Shift, (sdl_key_mods & KMOD_SHIFT) != 0);
        io.AddKeyEvent(ImGuiMod_Alt,   (sdl_key_mods & KMOD_ALT) != 0);
        io.AddKeyEvent(ImGuiMod_Super, (sdl_key_mods & KMOD_GUI) != 0);
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
    const float scale_x = (float)display_w / (float)w;
    const float scale_y = (float)display_h / (float)h;
    assert(scale_x == scale_y);
    set_screen_size({size_t(display_w), size_t(display_h), scale_x});
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2(scale_x, scale_y);
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
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_ShowCursor(SDL_FALSE);
    }
    else {
        // Show OS mouse cursor
        SDL_SetCursor(m_mouse_cursors[imgui_cursor] ? m_mouse_cursors[imgui_cursor] : m_mouse_cursors[ImGuiMouseCursor_Arrow]);
        SDL_ShowCursor(SDL_TRUE);
    }
}


double SDLRenderCanvas::platform_time()
{
    static Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 current_time = SDL_GetPerformanceCounter();
    return double(current_time) / frequency;
}

Render::Device& SDLRenderCanvas::device()
{
    return Render::Context::instance().device();
}

void SDLRenderCanvas::pass_event_to_scene(const SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        const auto &key = event.key;
        const KeyboardEvent::Type type = key.state == SDL_PRESSED ? KeyboardEvent::Type::KeyDown :
                                                                    KeyboardEvent::Type::KeyUp;
        // Platform independent keycode is mapped 1:1 to SDL KeyCode
        const auto code = KeyCode(key.keysym.sym);
        update_key_modifiers(type, code);

        KeyboardEvent platform_event{type, code, m_key_modifiers};
        enqueue_keyboard(platform_event);
    }
    else if (event.type == SDL_WINDOWEVENT_ENTER) {
        MouseEvent platform_event {
            MouseEvent::Type::Enter,
            MouseButton::NoButton,
            m_mouse_x, m_mouse_y,
            0, 0,
            m_key_modifiers
        };
        enqueue_mouse(platform_event);
    }
    else if (event.type == SDL_MOUSEMOTION) {
        const auto &mouse = event.motion;
        update_mouse_position(mouse.x, mouse.y);
        MouseEvent platform_event{
            MouseEvent::Type::Move, MouseButton::NoButton,
            mouse.x, mouse.y,
            0, 0,
            m_key_modifiers
        };
        enqueue_mouse(platform_event);
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
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
    else if (event.type == SDL_MOUSEWHEEL) {
        const auto& mouse_wheel = event.wheel;
        update_mouse_position(mouse_wheel.mouseX, mouse_wheel.mouseY);
        MouseEvent platform_event{
            MouseEvent::Type::Wheel, MouseButton::NoButton,
            m_mouse_x, m_mouse_y,
            mouse_wheel.preciseX, mouse_wheel.preciseY,
            m_key_modifiers
        };
        enqueue_mouse(platform_event);
    }
    else if (event.type == SDL_WINDOWEVENT_LEAVE) {
        MouseEvent platform_event {
            MouseEvent::Type::Leave,
            MouseButton::NoButton,
            m_mouse_x, m_mouse_y,
            0, 0,
            m_key_modifiers
        };
        enqueue_mouse(platform_event);
    }

}

}
