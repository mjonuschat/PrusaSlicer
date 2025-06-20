#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"

namespace Slic3r::App::Yoga {

class Circle;
class Text;

class RadioExtruder : public AbstractButton {
public:
    explicit RadioExtruder(size_t number, ImColor fill);

    const ImColor& fill() const;
    void set_fill(const ImColor& fill);

    size_t number() const;
    void set_number(size_t num);

    float border_width() const;
    void set_border_width(float border);

protected:
    void checked_updated_internal() override;

private:
    Circle* m_marker{ nullptr };
    Circle* m_knob{ nullptr };
    Text* m_text{ nullptr };

    ImColor m_fill{ IM_COL32_WHITE };
    float m_border_width{ 5 };
    size_t m_number_text{ 0 };
};

} // namespace Slic3r::App::Yoga 