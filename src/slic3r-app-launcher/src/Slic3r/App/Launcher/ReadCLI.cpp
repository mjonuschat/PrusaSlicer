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
    bool has_url = false;
    bool single_instance_on_url = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--no-gui") == 0)
            init_params.start_gui = false;
        else if (strcmp(argv[i], "--single-instance") == 0)
			init_params.single_instance = true;
		else if (strcmp(argv[i], "--no-single-instance") == 0)
			init_params.single_instance = false;
		else if (strcmp(argv[i], "--single-instance-on-url") == 0)
			single_instance_on_url = true;
        else if (strcmp(argv[i], "prusaslicer://") == 0)
			has_url = true;
    }
    if (has_url && single_instance_on_url) {
        init_params.single_instance = true;
    }
    return init_params;
}

} // namespace Slic3r::App::Launcher
