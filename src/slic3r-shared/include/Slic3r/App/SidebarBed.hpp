#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App {

class SidebarBed : public Yoga::Window
{
public:
    explicit SidebarBed(Yoga::Item* parent = nullptr);

    void render_body(Domain::Vec2f pos, Domain::Vec2f size) override;
};

} // namespace Slic3r::App
