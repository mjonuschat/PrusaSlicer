#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App::Plater {

class History : public Yoga::Window
{
public:
    explicit History(Item* parent = nullptr);
};

} // namespace Slic3r::App::Plater
