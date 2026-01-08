#pragma once

#include "Slic3r/App/Platform/KeyCode.hpp"
#include "Slic3r/App/Platform/KeyModifers.hpp"

#include <string>

namespace Slic3r::App::Platform {

struct KeyboardShortcut
{
    KeyModifiers modifiers;
    KeyCode key;

    std::string to_string() const;
    bool has_modifier() const;
};

} // Slic3r::App::Platform

