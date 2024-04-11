#include "main.hpp"

#include "App.hpp"
#include <slic3r3/Domain/Workbench.hpp>
#include <slic3r3/Domain/Bed.hpp>
#include "libslic3r/Model.hpp"
#include "slic3r3/App/Platform/RenderingPlatformImpl.hpp"

namespace Slic3r::App {

int slic3r_main(int argc, char **argv) {
    std::unique_ptr<Platform::IRenderingPlatform> rendering_platform = std::make_unique<Platform::RenderingPlatformImpl>();
    App app;

    rendering_platform->init();

    app.init(argc, argv);

    //  Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a
    // fopen() of the imgui.ini file. You may manually call LoadIniSettingsFromMemory() to load
    // settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!rendering_platform->should_quit())
#endif
    {
        rendering_platform->poll_events();
        app.render(*rendering_platform);
    }

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    return 0;
}

} // namespace Slic3r::App
