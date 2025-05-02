#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App::Preview {

class SidebarAutoReslice : public Yoga::Window
{
public:
    explicit SidebarAutoReslice(Item* parent = nullptr);

    void render_body(Domain::Vec2f pos, Domain::Vec2f size) override;
};

} // namespace Slic3r::App::Preview
