#pragma once

#include "Slic3r/App/Init.hpp"

namespace Slic3r::App::Launcher {

    InitParams read_cli(int argc, char* argv[]);

} // namespace Slic3r::App::Launcher
