#include <filesystem>
#include <boost/dll.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/nowide/cstdlib.hpp>


#include <Slic3r/Log.hpp>
#include <Slic3r/App/Init.hpp>
#include <Slic3r/App/Render/Context.hpp>
#include <Slic3r/App/Platform/PlatformServices.hpp>
#include <Slic3r/App/Platform/SDL/SDLRenderCanvas.hpp>
#include <libslic3r/Utils.hpp>
#include <libslic3r/Platform.hpp>
#include "Slic3r/App/TestRenderModule.hpp"

#ifdef WIN32
#include "Windows.h"
#endif


#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <functional>
#define EMSCRIPTEN_MAINLOOP_BEGIN       MainLoopForEmscriptenP = [&]()
#define EMSCRIPTEN_MAINLOOP_END         ; emscripten_set_main_loop(MainLoopForEmscripten, 0, true)
#else
#define EMSCRIPTEN_MAINLOOP_BEGIN
#define EMSCRIPTEN_MAINLOOP_END
#endif

#ifdef __EMSCRIPTEN__
#define WAIT_FOR_EVENT 0
#else
#define WAIT_FOR_EVENT 1
#endif

std::function<void()> main_loop_impl;
void main_loop()
{
    if (main_loop_impl)
        main_loop_impl();
}

void ls_directory(const std::filesystem::path& path)
{
    SPDLOG_INFO("Content of directory {}", path.string());
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            SPDLOG_INFO("{}", entry.path().string());
        }
    } else {
        SPDLOG_INFO("DIRECTORY {} DOES NOT EXIST!", path.string());
    }
}


void init_system()
{
    using namespace Slic3r;
    {
	    set_logging_level(4);
        const char *loglevel = boost::nowide::getenv("SLIC3R_LOGLEVEL");
        if (loglevel != nullptr) {
            if (loglevel[0] >= '0' && loglevel[0] <= '9' && loglevel[1] == 0)
                set_logging_level(loglevel[0] - '0');
            else
                boost::nowide::cerr << "Invalid SLIC3R_LOGLEVEL environment variable: " << loglevel << std::endl;
        }
    }

#ifdef __EMSCRIPTEN__
    ls_directory("/resources");
#endif
    // Detect the operating system flavor after SLIC3R_LOGLEVEL is set.
    detect_platform();

#ifdef WIN32
    // Notify user that a blacklisted DLL was injected into PrusaSlicer process (for example Nahimic, see GH #5573).
    // We hope that if a DLL is being injected into a PrusaSlicer process, it happens at the very start of the application,
    // thus we shall detect them now.
    //if (BlacklistedLibraryCheck::get_instance().perform_check()) {
    //    std::wstring text = L"Following DLLs have been injected into the PrusaSlicer process:\n\n";
    //    text += BlacklistedLibraryCheck::get_instance().get_blacklisted_string();
    //    text += L"\n\n"
    //            L"PrusaSlicer is known to not run correctly with these DLLs injected. "
    //            L"We suggest stopping or uninstalling these services if you experience "
    //            L"crashes or unexpected behaviour while using PrusaSlicer.\n"
    //            L"For example, ASUS Sonic Studio injects a Nahimic driver, which makes PrusaSlicer "
    //            L"to crash on a secondary monitor, see PrusaSlicer github issue #5573";
    //    MessageBoxW(NULL, text.c_str(), L"Warning"/*L"Incopatible library found"*/, MB_OK);
    //}
#endif
    App::init_paths();

}

std::unique_ptr<Slic3r::App::Platform::SDL::SDLRenderCanvas> canvas;
std::unique_ptr<Slic3r::App::TestRenderModule> render_module;

#ifdef WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
    Slic3r::set_logging_level(5);
    Slic3r::init_logging();

    init_system();

    canvas = std::make_unique<Slic3r::App::Platform::SDL::SDLRenderCanvas>();
    Slic3r::App::Render::Context::instance().log_gl_info();
    Slic3r::App::Platform::PlatformServices::instance().set_services(canvas.get(), canvas.get());
    render_module = std::make_unique<Slic3r::App::TestRenderModule>();
    SPDLOG_TRACE("RM created");
    canvas->set_render_module(render_module.get());
    SPDLOG_TRACE("RM registered");
#ifdef __EMSCRIPTEN__
    main_loop_impl = [&]()
#else
    while (!canvas->should_quit())
#endif
    {
        SPDLOG_TRACE("Loop start");
#if WAIT_FOR_EVENT
        canvas->wait_for_events();
#else
        canvas->poll_events();
#endif
        SPDLOG_TRACE("Loop render");
        canvas->render();
        SPDLOG_TRACE("Loop end");
    };
#ifdef __EMSCRIPTEN__
    SPDLOG_TRACE("Setting main loop");
    emscripten_set_main_loop(main_loop, 0, true);
#endif

    SPDLOG_TRACE("Quitting");
    return 0;
}
