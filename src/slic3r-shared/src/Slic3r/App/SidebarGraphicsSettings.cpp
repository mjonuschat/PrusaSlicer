#include "Slic3r/App/SidebarGraphicsSettings.hpp"
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"

#include "Slic3r/App/I18N/I18N.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

SidebarGraphicsSettings::SidebarGraphicsSettings() :
    Window("sidebar_graphics_settings")
{
    Item* panel = emplace_back<Yoga::Item>();
    panel->set_orientation(Yoga::Orientation::Vertical);
//    row->set_flex_shrink(0);
    panel->set_gap(5);

    Item* row = panel->emplace_back<Yoga::Item>();
    row->set_orientation(Yoga::Orientation::Horizontal);
    row->set_flex_shrink(0);
    row->set_gap(5);

    Text* text = row->emplace_back<Text>(L("Graphics settings"));
    text->set_width_percent(40);
    text->set_self_align(YGAlign::YGAlignCenter);
    text->set_font_type(Render::ImguiFontType::Bold);

    row = panel->emplace_back<Yoga::Item>();
    row->set_orientation(Yoga::Orientation::Horizontal);
    row->set_flex_shrink(0);
    row->set_gap(5);

    text = row->emplace_back<Text>(L("Shading type"));
    text->set_width_percent(30);
    text->set_self_align(YGAlign::YGAlignCenter);

    m_combo_settings = row->emplace_back<ComboBox>(Scene::SHADING_TYPE_NAMES);
    m_combo_settings->set_width_percent(70);
    m_combo_settings->set_self_align(YGAlign::YGAlignCenter);

    m_combo_settings->callbacks() = {
        nullptr,
        [&](int id) { Scene::Scene::graphics_settings().set_shading_type(Scene::ShadingType(id)); }
    };

    m_combo_settings->set_current_index(int(Scene::Scene::graphics_settings().shading_type()));

    m_show_debug_windows_btn = panel->emplace_back<Yoga::ToggleButton>(L("Show debug windows"));
    m_show_debug_windows_btn->set_checked(Scene::Scene::graphics_settings().debug_windows_enabled());
    m_show_debug_windows_btn->callbacks().checked_changed = [this](bool checked) {
        Scene::Scene::graphics_settings().set_debug_windows_enabled(checked);
    };
}

void SidebarGraphicsSettings::on_shading_type_changed(Scene::ShadingType shading_type)
{
    m_combo_settings->set_current_index(int(shading_type));
}

void SidebarGraphicsSettings::on_debug_windows_enabled_changed(bool enabled)
{
    m_show_debug_windows_btn->set_checked(enabled);
}

} // namespace Slic3r::App
