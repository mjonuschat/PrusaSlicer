#pragma once

#include "Slic3r/App/Yoga/RectangleButton.hpp"

namespace Slic3r::App::Yoga {

class Text;
class Circle;

class MaterialSettingsButton : public RectangleButton
{
public:
    explicit MaterialSettingsButton(size_t id, const std::string& tooltip = {});

    void set_color(const ImColor& color);
    void set_material_name(const std::string& name);
    void set_nozzle(float nozzle);

private:
    Circle* m_color_marker{ nullptr };
    Text* m_material_name{ nullptr };
    Text* m_nozzle{nullptr};
};

} // namespace Slic3r::App::Yoga
