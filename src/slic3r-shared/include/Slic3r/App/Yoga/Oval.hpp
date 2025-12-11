#pragma once

#include "Slic3r/App/Yoga/Rectangle.hpp"

namespace Slic3r::App::Yoga {

class Oval : public Rectangle {
public:
    explicit Oval();

    void render(Vec2f pos, Vec2f size) override;
};

} // namespace Slic3r::App::Yoga
