#include "Slic3r/App/Platform/WX/WXRenderCanvas.hpp"

#include <iostream>

#include <wx/dcclient.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <Slic3r/Biz/Platform/Termination.hpp>
#include <Slic3r/App/Platform/PlatformError.hpp>
#include <Slic3r/App/Render/Init.hpp>
#include <Slic3r/App/Render/Context.hpp>
#include <Slic3r/App/Render/Device.hpp>
#include <Slic3r/App/Render/Geometry.hpp>
#include <Slic3r/App/Render/Texture.hpp>
#include <Slic3r/Log.hpp>

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

    case WXK_CONTROL:
        return KeyCode::LCtrl;
    }

    wxChar unicode_char = event.GetUnicodeKey();
    if ('a' <= unicode_char && unicode_char <= 'z')
        unicode_char -= 'a' - 'A';
    if (' ' <= unicode_char && unicode_char <= 127)
        return KeyCode(unicode_char);
    return KeyCode::None;
}

static wxGLAttributes create_wxglattributes()
{
    //
    // TODO:
    // port missing settings from OpenGLManager::create_wxglcanvas
    //

    wxGLAttributes ret;
    ret.PlatformDefaults().DoubleBuffer()
        .RGBA().MinRGBA(8, 8, 8, 8)
        .Depth(24)
        .SampleBuffers(1).Samplers(4)
#ifdef __APPLE__
        // on MAC the method RGBA() has no effect
        .SetNeedsARB(true);
#else
    ;
#endif // __APPLE__
    ret.EndList();

    DEBUG_ASSERT(wxGLCanvas::IsDisplaySupported(ret));
    if (!wxGLCanvas::IsDisplaySupported(ret))
        ret.Reset();
    return ret;
}

WXRenderCanvas::WXRenderCanvas(wxWindow* parent)
: wxGLCanvas(parent, create_wxglattributes(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS), m_start_time(Clock::now())
{
    wxGLContextAttrs attrs;
    attrs.PlatformDefaults().CoreProfile();

#if defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    m_glsl_version = "#version 150";
    attrs.MajorVersion(3).MinorVersion(2);
#else
    // GL 3.0 + GLSL 130
    m_glsl_version = "#version 130";
    attrs.MajorVersion(3).MinorVersion(0);
#endif
    attrs.EndList();

    m_gl_context = std::make_unique<wxGLContext>(this, nullptr, &attrs);
    //SetCurrent(*m_gl_context);

    Bind(wxEVT_ENTER_WINDOW, &WXRenderCanvas::on_mouse_enter, this);
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
    Bind(wxEVT_LEAVE_WINDOW, &WXRenderCanvas::on_mouse_leave, this);
}

WXRenderCanvas::~WXRenderCanvas()
{ Render::shutdown_render(); }


void WXRenderCanvas::init()
{
    const auto err = glewInit();
    if (err != GLEW_NO_ERROR) {
        throw PlatformError(std::string("GLEW init failed with code ") + std::to_string(err));
    }

    glGetError();

    Render::initialize_render();

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
    ImGui_ImplOpenGL3_Init(m_glsl_version.c_str());
}

static ImGuiKey wx_to_imgui_key(int keycode)
{
    // 0..9
    if (48 <= keycode && keycode <= 57)
        return ImGuiKey(ImGuiKey_0 + keycode - 48);

    // A..Z
    if (65 <= keycode && keycode <= 90)
        return ImGuiKey(ImGuiKey_A + keycode - 65);

    // a..z
    if (97 <= keycode && keycode <= 122)
        return ImGuiKey(ImGuiKey_A + keycode - 97);

    switch (keycode)
    {
        case WXK_TAB:             return ImGuiKey_Tab;
        case WXK_LEFT:            return ImGuiKey_LeftArrow;
        case WXK_RIGHT:           return ImGuiKey_RightArrow;
        case WXK_UP:              return ImGuiKey_UpArrow;
        case WXK_DOWN:            return ImGuiKey_DownArrow;
        case WXK_PAGEUP:          return ImGuiKey_PageUp;
        case WXK_PAGEDOWN:        return ImGuiKey_PageDown;
        case WXK_HOME:            return ImGuiKey_Home;
        case WXK_END:             return ImGuiKey_End;
        case WXK_INSERT:          return ImGuiKey_Insert;
        case WXK_DELETE:          return ImGuiKey_Delete;
        case WXK_BACK:            return ImGuiKey_Backspace;
        case WXK_SPACE:           return ImGuiKey_Space;
        case WXK_RETURN:          return ImGuiKey_Enter;
        case WXK_ESCAPE:          return ImGuiKey_Escape;

        case '\'':                return ImGuiKey_Apostrophe;
        case ',':                 return ImGuiKey_Comma;
        case '-':                 return ImGuiKey_Minus;
        case '.':                 return ImGuiKey_Period;
        case '/':                 return ImGuiKey_Slash;
        case ';':                 return ImGuiKey_Semicolon;
        case '=':                 return ImGuiKey_Equal;
        case '[':                 return ImGuiKey_LeftBracket;
        case '\\':                return ImGuiKey_Backslash;
        case ']':                 return ImGuiKey_RightBracket;

        //case ?????: return ImGuiKey_GraveAccent;

        case WXK_CAPITAL:         return ImGuiKey_CapsLock;

        case WXK_SCROLL:          return ImGuiKey_ScrollLock;
        case WXK_NUMLOCK:         return ImGuiKey_NumLock;
        case WXK_PRINT:           return ImGuiKey_PrintScreen;
        case WXK_PAUSE:           return ImGuiKey_Pause;
        case WXK_NUMPAD0:         return ImGuiKey_Keypad0;
        case WXK_NUMPAD1:         return ImGuiKey_Keypad1;
        case WXK_NUMPAD2:         return ImGuiKey_Keypad2;
        case WXK_NUMPAD3:         return ImGuiKey_Keypad3;
        case WXK_NUMPAD4:         return ImGuiKey_Keypad4;
        case WXK_NUMPAD5:         return ImGuiKey_Keypad5;
        case WXK_NUMPAD6:         return ImGuiKey_Keypad6;
        case WXK_NUMPAD7:         return ImGuiKey_Keypad7;
        case WXK_NUMPAD8:         return ImGuiKey_Keypad8;
        case WXK_NUMPAD9:         return ImGuiKey_Keypad9;
        case WXK_NUMPAD_DELETE:   return ImGuiKey_KeypadDecimal;
        case WXK_NUMPAD_DIVIDE:   return ImGuiKey_KeypadDivide;
        case WXK_NUMPAD_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case WXK_NUMPAD_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case WXK_NUMPAD_ADD:      return ImGuiKey_KeypadAdd;
        case WXK_NUMPAD_ENTER:    return ImGuiKey_KeypadEnter;
        case WXK_NUMPAD_EQUAL:    return ImGuiKey_KeypadEqual;

        //case ?????: return ImGuiKey_LeftCtrl;
        //case ?????: return ImGuiKey_LeftShift;
        //case ?????: return ImGuiKey_LeftAlt;
        //case ?????: return ImGuiKey_LeftSuper;
        //case ?????: return ImGuiKey_RightCtrl;
        //case ?????: return ImGuiKey_RightShift;
        //case ?????: return ImGuiKey_RightAlt;
        //case ?????: return ImGuiKey_RightSuper;
        //case ?????: return ImGuiKey_Menu;

        case WXK_F1:              return ImGuiKey_F1;
        case WXK_F2:              return ImGuiKey_F2;
        case WXK_F3:              return ImGuiKey_F3;
        case WXK_F4:              return ImGuiKey_F4;
        case WXK_F5:              return ImGuiKey_F5;
        case WXK_F6:              return ImGuiKey_F6;
        case WXK_F7:              return ImGuiKey_F7;
        case WXK_F8:              return ImGuiKey_F8;
        case WXK_F9:              return ImGuiKey_F9;
        case WXK_F10:             return ImGuiKey_F10;
        case WXK_F11:             return ImGuiKey_F11;
        case WXK_F12:             return ImGuiKey_F12;
        case WXK_F13:             return ImGuiKey_F13;
        case WXK_F14:             return ImGuiKey_F14;
        case WXK_F15:             return ImGuiKey_F15;
        case WXK_F16:             return ImGuiKey_F16;
        case WXK_F17:             return ImGuiKey_F17;
        case WXK_F18:             return ImGuiKey_F18;
        case WXK_F19:             return ImGuiKey_F19;
        case WXK_F20:             return ImGuiKey_F20;
        case WXK_F21:             return ImGuiKey_F21;
        case WXK_F22:             return ImGuiKey_F22;
        case WXK_F23:             return ImGuiKey_F23;
        case WXK_F24:             return ImGuiKey_F24;

        //case ?????: return ImGuiKey_AppBack;
        //case ?????: return ImGuiKey_AppForward;

        default:                break;
    }
    return ImGuiKey_None;
}

void WXRenderCanvas::init_wx_imgui()
{
    ImGuiIO& io = ImGui::GetIO();

    // Don't let imgui special-case Mac, wxWidgets already do that
    io.ConfigMacOSXBehaviors = false;
}

void WXRenderCanvas::render()
{
    if (!IsShown())
        return;
    SetCurrent(*m_gl_context);
    if (!m_initialized) {
        init();
        m_initialized = true;
    }
    AbstractRenderCanvas::render();
}



void WXRenderCanvas::on_paint(wxPaintEvent& event)
{
    // This is a dummy, to avoid an endless succession of paint messages.
    // OnPaint handlers must always create a wxPaintDC.
    wxPaintDC dc(this);
    render();
}


void WXRenderCanvas::on_size(wxSizeEvent& event)
{
//    render();
}

KeyModifiers WXRenderCanvas::modifiers(const wxKeyboardState& event)
{
    KeyModifiers key_mods{0};

    if (event.ControlDown())
        key_mods |= KeyModifiers(KeyModifier::Ctrl);
    if (event.ShiftDown())
        key_mods |= KeyModifiers(KeyModifier::Shift);
    if (event.MetaDown())
        key_mods |= KeyModifiers(KeyModifier::Meta);
    if (event.AltDown())
        key_mods |= KeyModifiers(KeyModifier::Alt);

    return key_mods;
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

        ImGuiKey imgui_key = wx_to_imgui_key(key);
        if (imgui_key != ImGuiKey_None)
            io.AddKeyEvent(imgui_key, type == wxEVT_KEY_DOWN);
        io.AddKeyEvent(ImGuiMod_Ctrl,  evt.ControlDown());
        io.AddKeyEvent(ImGuiMod_Shift, evt.ShiftDown());
        io.AddKeyEvent(ImGuiMod_Alt,   evt.AltDown());
        io.AddKeyEvent(ImGuiMod_Super, evt.MetaDown());

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
                event_type, key_code, modifiers(evt)
            };

            enqueue_keyboard(platform_event);
        }
    }
    render();
}

void WXRenderCanvas::on_mouse_enter(wxMouseEvent& event)
{
    int mouse_x = ToDIP(event.GetX());
    int mouse_y = ToDIP(event.GetY());

    MouseEvent platform_event {
        MouseEvent::Type::Enter,
        MouseButton::NoButton,
        mouse_x, mouse_y,
        0, 0,
        modifiers(event)
    };
    enqueue_mouse(platform_event);
}

void WXRenderCanvas::on_mouse_leave(wxMouseEvent& event)
{
    int mouse_x = ToDIP(event.GetX());
    int mouse_y = ToDIP(event.GetY());

    MouseEvent platform_event {
        MouseEvent::Type::Leave,
        MouseButton::NoButton,
        mouse_x, mouse_y,
        0, 0,
        modifiers(event)
    };
    enqueue_mouse(platform_event);
}


void WXRenderCanvas::on_mouse(wxMouseEvent& evt)
{
    ImGuiIO& io = ImGui::GetIO();
    int mouse_x = ToDIP(evt.GetX());
    int mouse_y = ToDIP(evt.GetY());
    m_mouse_x = mouse_x;
    m_mouse_y = mouse_y;
    //int mouse_x = evt.GetX();
    //int mouse_y = evt.GetY();

    //SPDLOG_DEBUG("Mouse event {} {}", mouse_x, mouse_y);

    io.MousePos = ImVec2((float) mouse_x, (float) mouse_y);
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
    MouseButton button = MouseButton::NoButton;

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
        wheel_y = evt.GetWheelRotation();
        break;
    case wxMOUSE_WHEEL_HORIZONTAL:
        wheel_x = evt.GetWheelRotation();
        break;
    }

    MouseEvent platform_event{
        platform_event_type, button,
        mouse_x, mouse_y,
        wheel_x, wheel_y,
        modifiers(evt)
    };
    enqueue_mouse(platform_event);

    render();
}

void WXRenderCanvas::on_idle(wxIdleEvent& event)
{
    m_main_thread_dispatcher.dispatch_enqueued();
    bool render_requested = get_and_reset_render_requested();
    //std::cout << "Idle: render requested: " << render_requested << "\n";
    if (render_requested)
        render();
}

void WXRenderCanvas::begin_frame_platform()
{
    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    GetClientSize(&w, &h);
    // double scale_factor = wxWindow::GetContentScaleFactor();
    double scale_factor = wxWindow::GetDPIScaleFactor();
#if WIN32
    size_t display_w = w;
    size_t display_h = h;
    w /= scale_factor;
    h /= scale_factor;
#else
    size_t display_w = ToPhys(w);
    size_t display_h = ToPhys(h);
#endif
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    //SPDLOG_DEBUG("Setting screen resolution {} {} @ scale {} (phys {} {})", w, h, scale_factor, display_w, display_h);
    set_screen_size({display_w, display_h, float(scale_factor)});
    io.DisplayFramebufferScale = ImVec2(float(scale_factor), float(scale_factor));
}

void WXRenderCanvas::begin_imgui_frame_platform()
{
}

void WXRenderCanvas::end_imgui_frame_platform()
{

}

void WXRenderCanvas::end_frame_platform()
{
    wxGLCanvas::SwapBuffers();
    wxApp::GetInstance()->WakeUpIdle();
}

double WXRenderCanvas::platform_time()
{
    auto delta = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - m_start_time);
    return double(delta.count()) * 0.000001;
}

Render::Device& WXRenderCanvas::device()
{
    return Render::Context::instance().device();
}


void WXRenderCanvas::dispatch_on_main_thread(Biz::Platform::IMainThreadDispatcher::Function func)
{
    m_main_thread_dispatcher.dispatch_on_main_thread(std::move(func));
    wxApp::GetInstance()->WakeUpIdle();
}


} //namespace Slic3r::App::Platform::WX
