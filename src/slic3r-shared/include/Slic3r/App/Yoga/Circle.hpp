#pragma once

#include "Slic3r/App/Yoga/Rectangle.hpp"

namespace Slic3r::App::Yoga {

class Circle : public Yoga::Rectangle
{
public:
    explicit Circle();

private:
    void render(Vec2f pos, Vec2f size) override;
};

} // namespace Slic3r::App::Yoga
