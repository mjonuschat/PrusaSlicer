#pragma once

#include "Slic3r/App/Yoga/Window.hpp"
#include "Slic3r/App/Scene/GraphicsSettings.hpp"

namespace Slic3r::App {

namespace Yoga {
class ComboBox;
class ToggleButton;
} // namespace Yoga

class SidebarGraphicsSettings : public Yoga::Window, public Scene::IGraphicsSettingsChangedListener
{
public:
    SidebarGraphicsSettings();

    void on_shading_type_changed(Scene::ShadingType shading_type) override;
    void on_debug_windows_enabled_changed(bool enabled) override;

private:
    Yoga::ComboBox* m_combo_settings{nullptr};
    Yoga::ToggleButton* m_show_debug_windows_btn{ nullptr };
};

} // namespace Slic3r::App
