#pragma once

#include <Slic3r/App/Platform/KeyCode.hpp>
#include <Slic3r/App/Platform/KeyModifers.hpp>

namespace Slic3r::App {

struct KeyboardShortcut
{
    Platform::KeyModifiers modifiers;
    Platform::KeyCode key;
};

}