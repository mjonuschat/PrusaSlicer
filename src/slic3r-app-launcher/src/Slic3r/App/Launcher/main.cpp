#ifdef _WIN32
    #include <windows.h>
#endif
#ifdef SLIC3R_GUI
    #include "Slic3r/App/Desktop/Run.hpp"
#endif

#include "Slic3r/App/CLI/CLIApp.hpp"
#include "ReadCLI.hpp"
#include "Slic3r/Log.hpp"

#include "boost/nowide/args.hpp"
#include "boost/nowide/convert.hpp"

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif

#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)

extern "C" {

    const char* __lsan_default_suppressions();
    const char* __lsan_default_suppressions() {
        return "leak:libfontconfig\n"              // FontConfig looks like it leaks, but it doesn't.
            ;
    }

}

#endif


int main(int argc, char* argv[])
{
    boost::nowide::args args(argc, argv); // RAII converting argv to UTF-8.

    Slic3r::App::InitParams init_params = Slic3r::App::Launcher::read_cli(argc, argv);

    if (init_params.start_gui) {
        #ifdef SLIC3R_GUI
            return Slic3r::App::Desktop::run(init_params);
        #endif
        SPDLOG_ERROR("ERROR: PrusaSlicer was built without GUI support. Quitting.");
        return 1;        
    }
    return Slic3r::App::CLI::run(init_params);
}

// This is currently not used, see respective add_executable in CMake.
// #ifdef _WIN32
//int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
//{
//    return main(__argc, __argv);
//}
//#endif
