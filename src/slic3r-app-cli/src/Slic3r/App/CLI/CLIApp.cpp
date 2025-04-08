#include "Slic3r/Log.hpp"

#include "Slic3r/App/Init.hpp"

namespace Slic3r::App::CLI {

int run(const InitParams& init_params)
{
    // TODO: Port CLI application logic. Argument parsing should be done first in ReadCLI.cpp,
    // InitParams should contain all information.
    SPDLOG_ERROR("CLIApp::run called, but CLI does not do anything yet. Goodbye.");
    return 0;
}
} // namespace Slic3r::App::CLI