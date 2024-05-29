#include <boost/dll.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/nowide/cstdlib.hpp>
#include <spdlog/spdlog.h>

#include <Slic3r/App/Platform/PlatformServices.hpp>
#include <Slic3r/App/Platform/SDL/SDLRenderCanvas.hpp>
#include <libslic3r/Utils.hpp>
#include <libslic3r/Platform.hpp>
#include "Slic3r/App/TestRenderModule.hpp"

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

    // Detect the operating system flavor after SLIC3R_LOGLEVEL is set.
    detect_platform();

#ifdef WIN32
    // Notify user that a blacklisted DLL was injected into PrusaSlicer process (for example Nahimic, see GH #5573).
    // We hope that if a DLL is being injected into a PrusaSlicer process, it happens at the very start of the application,
    // thus we shall detect them now.
    if (BlacklistedLibraryCheck::get_instance().perform_check()) {
        std::wstring text = L"Following DLLs have been injected into the PrusaSlicer process:\n\n";
        text += BlacklistedLibraryCheck::get_instance().get_blacklisted_string();
        text += L"\n\n"
                L"PrusaSlicer is known to not run correctly with these DLLs injected. "
                L"We suggest stopping or uninstalling these services if you experience "
                L"crashes or unexpected behaviour while using PrusaSlicer.\n"
                L"For example, ASUS Sonic Studio injects a Nahimic driver, which makes PrusaSlicer "
                L"to crash on a secondary monitor, see PrusaSlicer github issue #5573";
        MessageBoxW(NULL, text.c_str(), L"Warning"/*L"Incopatible library found"*/, MB_OK);
    }
#endif

#ifdef __EMSCRIPTEN__
    boost::filesystem::path path_resources = "/resources";
#else
    // See Invoking prusa-slicer from $PATH environment variable crashes #5542
    // boost::filesystem::path path_to_binary = boost::filesystem::system_complete(argv[0]);
    boost::filesystem::path path_to_binary = boost::dll::program_location();

    // Path from the Slic3r binary to its resources.
#ifdef __APPLE__
    // The application is packed in the .dmg archive as 'Slic3r.app/Contents/MacOS/Slic3r'
    // The resources are packed to 'Slic3r.app/Contents/Resources'
    boost::filesystem::path path_resources = boost::filesystem::canonical(path_to_binary).parent_path() / "../Resources";
#elif defined _WIN32
    // The application is packed in the .zip archive in the root,
    // The resources are packed to 'resources'
    // Path from Slic3r binary to resources:
    boost::filesystem::path path_resources = path_to_binary.parent_path() / "resources";
#elif defined SLIC3R_FHS
    // The application is packaged according to the Linux Filesystem Hierarchy Standard
    // Resources are set to the 'Architecture-independent (shared) data', typically /usr/share or /usr/local/share
    boost::filesystem::path path_resources = SLIC3R_FHS_RESOURCES;
#else
    // The application is packed in the .tar.bz archive (or in AppImage) as 'bin/slic3r',
    // The resources are packed to 'resources'
    // Path from Slic3r binary to resources:
    boost::filesystem::path path_resources = boost::filesystem::canonical(path_to_binary).parent_path() / "../resources";
#endif

#endif // __EMSCRIPTEN__

    set_resources_dir(path_resources.string());
    set_var_dir((path_resources / "icons").string());
    set_local_dir((path_resources / "localization").string());
    set_sys_shapes_dir((path_resources / "shapes").string());
    set_custom_gcodes_dir((path_resources / "custom_gcodes").string());
}

int main(int argc, char** argv)
{
    std::cerr << "Hello" << std::endl;
    Slic3r::set_logging_level(5);
    std::cerr << "Hello" << std::endl;
    SPDLOG_INFO("Hello from spdlog");
    SPDLOG_ERROR("Error from spdlog");

    init_system();

    Slic3r::App::Platform::SDL::SDLRenderCanvas canvas;
    Slic3r::App::Platform::PlatformServices::instance().set_services(&canvas, &canvas);
    Slic3r::App::TestRenderModule render_module;
    SPDLOG_INFO("RM created");
    canvas.set_render_module(&render_module);
    SPDLOG_INFO("RM registered");
#ifdef __EMSCRIPTEN__
    main_loop_impl = [&]()
#else
    while (!canvas.should_quit())
#endif
    {
        SPDLOG_INFO("Loop start");
#if WAIT_FOR_EVENT
        canvas.wait_for_events();
#else
        canvas.poll_events();
#endif
        SPDLOG_INFO("Loop render");
        canvas.render();
        SPDLOG_INFO("Loop end");
    };
#ifdef __EMSCRIPTEN__
    SPDLOG_INFO("Setting main loop");
    emscripten_set_main_loop(main_loop, 0, true);
#endif

    return 0;
}
