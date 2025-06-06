#pragma once

#include "Slic3r/App/Yoga/Rectangle.hpp"

namespace Slic3r::App::Yoga {

class ProjectButtonBackground : public Rectangle
{
public: 
    enum Mode {
        Border,
        FilledRect
    };

    ProjectButtonBackground();
    void render(Vec2f pos, Vec2f size) override;

    Mode mode() const;
    void set_mode(Mode mode);

    float inner_rounding() const;
    void set_inner_rounding(float rounding);

    Vec2f thickness() const;
    void set_thichness(Vec2f thickness);

private:
    Mode m_mode{ FilledRect };
    float m_inner_rounding{5.f};
    Vec2f m_thickness{ 7.f, 5.f };
};

} // namespace Slic3r::App::Yoga