#pragma once

#include <optional>

namespace Slic3r::App {

extern void init_paths();

class InitParams {
public:
    // TODO: Add actual InitParams. The object is passed to either CLI and GUI applications
    // so they can decide what to do after startup. This needs careful porting from old
    // slicer, not doing from scratch.
    bool start_gui = true;
    int argc = 0;
    char** argv = nullptr;
    std::optional<bool>	single_instance;
};

}
