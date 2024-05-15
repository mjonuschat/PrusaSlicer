#include "WXRenderCanvas.hpp"

#include <iostream>

#include <wx/dcclient.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "Slic3r/App/Platform/PlatformError.hpp"

namespace Slic3r::App::Platform::WX {

KeyCode get_key_code_from_event(const wxKeyEvent& event)
{
    switch (event.GetKeyCode())
    {
    case WXK_NONE:
        return KeyCode::None;

    case WXK_RETURN:
        return KeyCode::Return;

    case WXK_ESCAPE:
        return KeyCode::Escape;

    case WXK_BACK:
        return KeyCode::Backspace;

    case WXK_TAB:
        return KeyCode::Tab;

    case WXK_SPACE:
        return KeyCode::Space;

    case WXK_F1:
        return KeyCode::F1;

    case WXK_F2:
        return KeyCode::F2;

    case WXK_F3:
        return KeyCode::F3;

    case WXK_F4:
        return KeyCode::F4;

    case WXK_F5:
        return KeyCode::F5;

    case WXK_F6:
        return KeyCode::F6;

    case WXK_F7:
        return KeyCode::F7;

    case WXK_F8:
        return KeyCode::F8;

    case WXK_F9:
        return KeyCode::F9;

    case WXK_F10:
        return KeyCode::F10;

    case WXK_F11:
        return KeyCode::F11;

    case WXK_F12:
        return KeyCode::F12;

    case WXK_PAUSE:
        return KeyCode::Pause;

    case WXK_INSERT:
        return KeyCode::Insert;

    case WXK_HOME:
        return KeyCode::Home;

    case WXK_PAGEUP:
        return KeyCode::PageUp;

    case WXK_DELETE:
        return KeyCode::Delete;

    case WXK_END:
        return KeyCode::End;

    case WXK_PAGEDOWN:
        return KeyCode::PageDown;

    case WXK_RIGHT:
        return KeyCode::Right;

    case WXK_LEFT:
        return KeyCode::Left;

    case WXK_DOWN:
        return KeyCode::Down;

    case WXK_UP:
        return KeyCode::Up;

    case WXK_NUMPAD_DIVIDE:
        return KeyCode::KpDivide;

    case WXK_NUMPAD_MULTIPLY:
        return KeyCode::KpMultiply;

    case WXK_NUMPAD_SUBTRACT:
        return KeyCode::KpMinus;

    case WXK_NUMPAD_ADD:
        return KeyCode::KpPlus;

    case WXK_NUMPAD_ENTER:
        return KeyCode::KpEnter;

    case WXK_NUMPAD1:
        return KeyCode::Kp1;

    case WXK_NUMPAD2:
        return KeyCode::Kp2;

    case WXK_NUMPAD3:
        return KeyCode::Kp3;

    case WXK_NUMPAD4:
        return KeyCode::Kp4;

    case WXK_NUMPAD5:
        return KeyCode::Kp5;

    case WXK_NUMPAD6:
        return KeyCode::Kp6;

    case WXK_NUMPAD7:
        return KeyCode::Kp7;

    case WXK_NUMPAD8:
        return KeyCode::Kp8;

    case WXK_NUMPAD9:
        return KeyCode::Kp9;

    case WXK_NUMPAD0:
        return KeyCode::Kp0;

    case WXK_NUMPAD_SEPARATOR:
        return KeyCode::KpPeriod;

    case WXK_F13:
        return KeyCode::F13;

    case WXK_F14:
        return KeyCode::F14;

    case WXK_F15:
        return KeyCode::F15;

    case WXK_F16:
        return KeyCode::F16;

    case WXK_F17:
        return KeyCode::F17;

    case WXK_F18:
        return KeyCode::F18;

    case WXK_F19:
        return KeyCode::F19;

    case WXK_F20:
        return KeyCode::F20;

    case WXK_F21:
        return KeyCode::F21;

    case WXK_F22:
        return KeyCode::F22;

    case WXK_F23:
        return KeyCode::F23;

    case WXK_F24:
        return KeyCode::F24;

    case WXK_EXECUTE:
        return KeyCode::Execute;

    case WXK_HELP:
        return KeyCode::Help;

    case WXK_MENU:
        return KeyCode::Menu;

    case WXK_SELECT:
        return KeyCode::Select;

    case WXK_CANCEL:
        return KeyCode::Cancel;

    case WXK_CLEAR:
        return KeyCode::Clear;

    case WXK_SEPARATOR:
        return KeyCode::Separator;

    case WXK_NUMPAD_TAB:
        return KeyCode::KpTab;

    case WXK_NUMPAD_SPACE:
        return KeyCode::KpSpace;

    case WXK_NUMPAD_DECIMAL:
        return KeyCode::KpPeriod;

    case WXK_SHIFT:
        return KeyCode::LShift;

    case WXK_ALT:
        return KeyCode::LAlt;
    }

    wxChar unicode_char = event.GetUnicodeKey();
    if ('a' <= unicode_char && unicode_char <= 'z')
        unicode_char -= 'a' - 'A';
    if (' ' <= unicode_char && unicode_char <= 127)
        return KeyCode(unicode_char);
    return KeyCode::None;
}

WXRenderCanvas::WXRenderCanvas(wxWindow* parent)
: wxGLCanvas(parent), m_start_time(Clock::now())
{
    wxGLContextAttrs attrs;
    attrs.PlatformDefaults()
        .CoreProfile()
        //.RGBA()
        //.DoubleBuffer()
        //.MinRGBA(8,8,8,8)
        //.Depth(24)
        .EndList();

#ifdef __APPLE__
    // on MAC the method RGBA() has no effect
    attrs.SetNeedsARB(true);
#endif // __APPLE__

#if defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    attrs.MajorVersion(3).MinorVersion(2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    attrs.MajorVersion(3).MinorVersion(0);
#endif

    m_gl_context = std::make_unique<wxGLContext>(this, nullptr, &attrs);
    SetCurrent(*m_gl_context);

    const auto err = glewInit();
    if (err != GLEW_NO_ERROR) {
        throw PlatformError(std::string("GLEW init failed with code ") + std::to_string(err));
    }
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    init_wx_imgui();
    ImGui_ImplOpenGL3_Init(glsl_version);

    Bind(wxEVT_SIZE, &WXRenderCanvas::on_size, this);
    Bind(wxEVT_IDLE, &WXRenderCanvas::on_idle, this);
    Bind(wxEVT_PAINT, &WXRenderCanvas::on_paint, this);
    Bind(wxEVT_CHAR, &WXRenderCanvas::on_keyboard, this);
    Bind(wxEVT_KEY_DOWN, &WXRenderCanvas::on_keyboard, this);
    Bind(wxEVT_KEY_UP, &WXRenderCanvas::on_keyboard, this);
    Bind(wxEVT_MOUSEWHEEL, &WXRenderCanvas::on_mouse, this);
    Bind(wxEVT_MOTION, &WXRenderCanvas::on_mouse, this);
    Bind(wxEVT_LEFT_DOWN, &WXRenderCanvas::on_mouse, this);
    Bind(wxEVT_LEFT_UP, &WXRenderCanvas::on_mouse, this);
    Bind(wxEVT_RIGHT_DOWN, &WXRenderCanvas::on_mouse, this);
    Bind(wxEVT_RIGHT_UP, &WXRenderCanvas::on_mouse, this);
    Bind(wxEVT_MIDDLE_DOWN, &WXRenderCanvas::on_mouse, this);
    Bind(wxEVT_MIDDLE_UP, &WXRenderCanvas::on_mouse, this);
}


void WXRenderCanvas::init_wx_imgui()
{
    ImGuiIO& io = ImGui::GetIO();

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = WXK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = WXK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = WXK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = WXK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = WXK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = WXK_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = WXK_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = WXK_HOME;
    io.KeyMap[ImGuiKey_End] = WXK_END;
    io.KeyMap[ImGuiKey_Insert] = WXK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = WXK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = WXK_BACK;
    io.KeyMap[ImGuiKey_Space] = WXK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = WXK_RETURN;
    io.KeyMap[ImGuiKey_KeyPadEnter] = WXK_NUMPAD_ENTER;
    io.KeyMap[ImGuiKey_Escape] = WXK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

    // Don't let imgui special-case Mac, wxWidgets already do that
    io.ConfigMacOSXBehaviors = false;
}


void WXRenderCanvas::on_paint(wxPaintEvent& event)
{
    // This is a dummy, to avoid an endless succession of paint messages.
    // OnPaint handlers must always create a wxPaintDC.
    wxPaintDC dc(this);
    SetCurrent(*m_gl_context);
    render();
}


void WXRenderCanvas::on_size(wxSizeEvent& event)
{
//    render();
}

void WXRenderCanvas::on_keyboard(wxKeyEvent& evt)
{
    wxEventType type = evt.GetEventType();
    ImGuiIO& io = ImGui::GetIO();

    if (type == wxEVT_CHAR) {
        // Char event
        const auto   key   = evt.GetUnicodeKey();

        // Release BackSpace, Delete, ... when miss wxEVT_KEY_UP event
        // Already Fixed at begining of new frame
        // unsigned int key_u = static_cast<unsigned int>(key);
        //if (key_u >= 0 && key_u < IM_ARRAYSIZE(io.KeysDown) && io.KeysDown[key_u]) {
        //    io.KeysDown[key_u] = false;
        //}

        if (key != 0) {
            io.AddInputCharacter(key);
        }
    } else if (type == wxEVT_KEY_DOWN || type == wxEVT_KEY_UP) {
        // Key up/down event
        int key = evt.GetKeyCode();
        //wxCHECK_MSG(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown), false, "Received invalid key code");

        io.KeysDown[key] = (type == wxEVT_KEY_DOWN);
        io.KeyShift = evt.ShiftDown();
        io.KeyCtrl = evt.ControlDown();
        io.KeyAlt = evt.AltDown();
        io.KeySuper = evt.MetaDown();

        if (key != WXK_TAB
            && key != WXK_LEFT
            && key != WXK_UP
            && key != WXK_RIGHT
            && key != WXK_DOWN) {
            evt.Skip();   // Needed to have EVT_CHAR generated as well
        }

        KeyCode key_code = get_key_code_from_event(evt);
        if (key_code != KeyCode::None) {

            KeyboardEvent::Type event_type = type == wxEVT_KEY_DOWN
                ? KeyboardEvent::Type::KeyDown
                : KeyboardEvent::Type::KeyUp;

            update_key_modifiers(event_type, key_code);

            KeyboardEvent platform_event{
                event_type, key_code, m_key_modifiers
            };

            enqueue_keyboard(platform_event);
        }
    }
    render();
}

void WXRenderCanvas::on_mouse(wxMouseEvent& evt)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)evt.GetX(), (float)evt.GetY());
    io.MouseDown[0] = evt.LeftIsDown();
    io.MouseDown[1] = evt.RightIsDown();
    io.MouseDown[2] = evt.MiddleIsDown();
    io.MouseDoubleClicked[0] = evt.LeftDClick();
    io.MouseDoubleClicked[1] = evt.RightDClick();
    io.MouseDoubleClicked[2] = evt.MiddleDClick();
    float wheel_delta = static_cast<float>(evt.GetWheelDelta());
    if (wheel_delta != 0.0f)
        io.MouseWheel = static_cast<float>(evt.GetWheelRotation()) / wheel_delta;

    wxEventType event_type = evt.GetEventType();

    MouseEvent::Type platform_event_type = MouseEvent::Type::Move;
    MouseButton button = MouseButton::None;

    if (event_type == wxEVT_MOUSEWHEEL)
        platform_event_type = MouseEvent::Type::Wheel;
    else if (event_type == wxEVT_LEFT_DOWN) {
        platform_event_type = MouseEvent::Type::ButtonDown;
        button = MouseButton::Left;
    } else if (event_type == wxEVT_LEFT_UP) {
        platform_event_type = MouseEvent::Type::ButtonUp;
        button = MouseButton::Left;
    } else if (event_type == wxEVT_RIGHT_DOWN) {
        platform_event_type = MouseEvent::Type::ButtonDown;
        button = MouseButton::Right;
    } else if (event_type == wxEVT_RIGHT_UP) {
        platform_event_type = MouseEvent::Type::ButtonUp;
        button = MouseButton::Right;
    } else if (event_type == wxEVT_MIDDLE_DOWN) {
        platform_event_type = MouseEvent::Type::ButtonDown;
        button = MouseButton::Middle;
    } else if (event_type == wxEVT_MIDDLE_UP) {
        platform_event_type = MouseEvent::Type::ButtonUp;
        button = MouseButton::Middle;
    }

    float wheel_x = 0;
    float wheel_y = 0;
    switch (evt.GetWheelAxis()) {
    case wxMOUSE_WHEEL_VERTICAL:
        wheel_y = evt.GetWheelDelta();
        break;
    case wxMOUSE_WHEEL_HORIZONTAL:
        wheel_x = evt.GetWheelDelta();
        break;
    }


    MouseEvent platform_event{
        platform_event_type, button, evt.GetX(), evt.GetY(), wheel_x, wheel_y, m_key_modifiers
    };
    enqueue_mouse(platform_event);

    render();
}

void WXRenderCanvas::on_idle(wxIdleEvent& event)
{
    dispatch_enqueued();
    bool render_requested = get_and_reset_render_requested();
    std::cout << "Idle: render requested: " << render_requested << "\n";
    if (render_requested)
        render();
}


void WXRenderCanvas::begin_frame_platform()
{
}

void WXRenderCanvas::begin_imgui_frame_platform()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    GetClientSize(&w, &h);
//    SDL_GL_GetDrawableSize(m_window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    double scale_factor = wxWindow::GetContentScaleFactor();
    io.DisplayFramebufferScale = ImVec2(float(scale_factor), float(scale_factor));
    /*
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);

    update_imgui_mouse_position();
    update_imgui_mouse_cursor();
    */
}

void WXRenderCanvas::end_imgui_frame_platform()
{

}

void WXRenderCanvas::end_frame_platform()
{
    wxGLCanvas::SwapBuffers();
    wxApp::GetInstance()->WakeUpIdle();
}

double WXRenderCanvas::get_platform_time()
{
    auto delta = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - m_start_time);
    return double(delta.count()) * 0.000001;
}

void WXRenderCanvas::dispatch_on_main_thread(IMainThreadDispatcher::Function func)
{
    AbstractRenderCanvas::dispatch_on_main_thread(func);
    wxApp::GetInstance()->WakeUpIdle();
}


} //namespace Slic3r::App::Platform::WX
