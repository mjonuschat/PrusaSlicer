#include "ReadCLI.hpp"

#include <string.h>

namespace Slic3r::App::Launcher {

InitParams read_cli(int argc, char* argv[])
{
    InitParams init_params;
    init_params.argc = argc;
    init_params.argv = argv;

    // TODO: Add actual handling of command line arguments.
    // Most of it is already done in the old slicer, it is enough to port it.
    // For now, we only check for the --no-gui flag.

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-gui") == 0)
            init_params.start_gui = false;
    }
    return init_params;
}

} // namespace Slic3r::App::Launcher
