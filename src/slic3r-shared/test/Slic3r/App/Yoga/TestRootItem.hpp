///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/App/Yoga/RootItem.hpp>

namespace Slic3r::App::Yoga {

class TestRootItem : public RootItem
{
public:
    void process_loop_events() { m_loop_events.process_events(); }
};

} // namespace Slic3r::App::Yoga
