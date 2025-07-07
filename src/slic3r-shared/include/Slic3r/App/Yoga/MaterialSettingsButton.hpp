#pragma once

#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include "Slic3r/App/MaterialState.hpp"

namespace Slic3r::App {
class FilamentSettingsDialog;
}

namespace Slic3r::App::Yoga {

class Text;
class Circle;
class ButtonGroup;

class MaterialSettingsButton : public RectangleButton, public Biz::DataObserver<MaterialState>
{
public:
    MaterialSettingsButton(
        size_t index,
        const MaterialState& state,
        std::weak_ptr<ButtonGroup> button_group
    );
    ~MaterialSettingsButton();

    void set_color(const ImColor& color);
    void set_material_name(const std::string& name);
    void set_nozzle(float nozzle);

    void on_data_update() override;

private:
    Circle* m_color_marker{nullptr};
    Text* m_material_name{nullptr};
    Text* m_nozzle{nullptr};
    std::weak_ptr<ButtonGroup> m_button_group;
};

} // namespace Slic3r::App::Yoga
