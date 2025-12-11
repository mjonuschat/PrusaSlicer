#pragma once

#include "Slic3r/App/Yoga/Oval.hpp"

namespace Slic3r::App::Yoga {

class Circle;

class Toggler : public Oval
{
public:
    Toggler();

    void style_node() override;

    void set_checked(bool checked);

    bool third_state() const;
    void set_third_state(bool third_state);

protected:
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
