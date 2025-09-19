#pragma once

#include "Slic3r/App/Yoga/Oval.hpp"

namespace Slic3r::App::Yoga {

class Circle;

class Toggler : public Oval
{
public:
    Toggler();
    void set_checked(bool checked);

    bool third_state() const;
    void set_third_state(bool third_state);

protected:
    void process_events(Vec2f pos, Vec2f size) override;

    void update_contents();

    ImColor bg_color(bool hovered) const;
    ImColor knob_color() const;

private:
    bool m_third_state = false;
    bool m_checked     = false;
    Circle* m_knob{nullptr};
    Oval* m_inner_oval{nullptr};
};

} // namespace Slic3r::App::Yoga
