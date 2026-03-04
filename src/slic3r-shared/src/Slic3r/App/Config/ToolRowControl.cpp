///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ToolRowControl.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/IConfigBoxSetter.hpp"

#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Config/ConfigItemPreview.hpp"
#include "Slic3r/App/Config/ConfigItemUtils.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ToolRowControl::ToolRowControl(
    size_t index,
    const ToolRowOverride& data,
    Biz::IConfigBoxSetter& cb_setter
) :
    Biz::DataObserver<ToolRowOverride>(index, data),
    m_cb_setter(cb_setter)
{
    set_orientation(Orientation::Horizontal);
    set_gap(5);
    set_align_items(YGAlign::YGAlignCenter);
    set_height(29);

    m_icon = emplace_back<Icon>(Render::Icon::Funnel);
    m_icon->set_width(18);
    m_icon->set_height(18);

    m_label = emplace_back<Text>(std::string{});

    m_switch_override = emplace_back<LayoutButton>(std::string{}, Render::Icon::Chain);
    m_switch_override->set_width(22);
    m_switch_override->set_height(22);
    m_switch_override->callbacks().action = [this]
    { m_cb_setter.set_item_override(*m_state->override_item, !m_state->overriden, m_index); };

    m_default_label = emplace_back<Text>(Biz::_u8L("(default)"));
    m_default_label->set_font_type(Render::ImguiFontType::Italic);
    m_default_label->set_visible(false);
    m_default_label->set_enabled(false);

    m_in_use_label = emplace_back<Text>(Biz::_u8L("(in use)"));
    m_in_use_label->set_font_type(Render::ImguiFontType::Italic);
    m_in_use_label->set_visible(false);
    m_in_use_label->set_enabled(false);

    on_index_update();
    on_data_update();
}

void ToolRowControl::on_data_update()
{
    bool overriden = m_state->overriden;

    if (m_state->print_item->name() == "nozzle_high_flow") {
        ASSERT(true);
    }

    m_switch_override->set_icon(overriden ? Render::Icon::Unchain : Render::Icon::Chain);
    m_switch_override->set_tooltip(
        overriden ? Biz::_u8L("Use default value") : Biz::_u8L("Use tool specific value")
    );
    m_switch_override->set_tooltip_position(Position::Top);

    const ImColor text_color     = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    const ImColor disabled_color = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);

    m_switch_override->set_icon_tint(overriden ? text_color : disabled_color);
    const Domain::ConfigItem* config_item = m_state->override_item;
    if (m_last_overriden != overriden || m_control_gui_type != config_item->def().gui_type) {
        m_last_overriden = overriden;
        // We got a new ConfigItem assigned with different GuiType
        if (m_input) {
            m_control_gui_type = Domain::ConfigItemDef::GUIType::undefined;
            remove(m_input);
            m_input   = nullptr;
            m_control = nullptr;
        }
        if (m_preview) {
            remove(m_preview);
            m_preview = nullptr;
        }

        if (overriden) {
            m_control_gui_type = config_item->def().gui_type;
            m_control          = ConfigItemControl::config_item_control_factory(
                this,
                3,
                m_index,
                *config_item,
                m_cb_setter,
                m_index
            );

            m_input = dynamic_cast<Yoga::Item*>(m_control);
            ASSERT(m_input, "ConfigItem needs to derive from Yoga::Item");
        } else {
            std::unique_ptr<ConfigItemPreview> preview = std::make_unique<ConfigItemPreview>();
            m_preview                                  = preview.get();
            insert(std::move(preview), 3);
        }
    }

    ImColor color =
        m_state->extruder_candidate ? ConfigItemUtils::colors.at(m_index) : disabled_color;
    m_icon->set_tint(color);
    m_label->set_text_color(color);

    m_default_label->set_visible(!overriden);
    m_in_use_label->set_visible(m_state->extruder_candidate);
    if (m_control) {
        m_control->set_state(*m_state->override_item);
    } else if (m_preview) {
        m_preview->set_data(*m_state->print_item, m_state->print_item->value(), false);
    }
}

void ToolRowControl::on_index_update()
{
    m_label->set_text(Biz::_u8L("Tool") + " " + std::to_string(m_index + 1));
}

} // namespace Slic3r::App
