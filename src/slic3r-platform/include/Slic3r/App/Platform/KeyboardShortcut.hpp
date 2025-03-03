#pragma once

#include "Slic3r/App/Platform/KeyCode.hpp"
#include "Slic3r/App/Platform/KeyModifers.hpp"

namespace Slic3r::App::Platform {

struct KeyboardShortcut
{
    KeyModifiers modifiers;
    KeyCode key;
};

} // Slic3r::App::Platform

