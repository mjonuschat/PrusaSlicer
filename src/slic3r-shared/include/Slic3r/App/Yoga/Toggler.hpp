#pragma once

#include "Slic3r/App/Yoga/Oval.hpp"

namespace Slic3r::App::Yoga {

class Circle;

class Toggler : public Oval
{
public:
    Toggler();
    void set_checked(bool checked);

protected:
    void process_events(Vec2f pos, Vec2f size) override;

private:
    Circle* m_knob { nullptr };
};

} // namespace Slic3r::App::Yoga