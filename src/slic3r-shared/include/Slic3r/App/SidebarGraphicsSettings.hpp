#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Scene/GraphicsSettings.hpp"

namespace Slic3r::App {

namespace Yoga {
class ComboBox;
} // namespace Yoga

class SidebarGraphicsSettings : public Yoga::Window, public Scene::IGraphicsSettingsChangedListener
{
public:
    SidebarGraphicsSettings();

    void on_shading_type_changed(Scene::ShadingType shading_type) override;

private:
    Yoga::ComboBox* m_combo_settings{nullptr};
};

} // namespace Slic3r::App
