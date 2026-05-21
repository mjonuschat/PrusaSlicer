///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigRowItem.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Config/ConfigItemSpinBox.hpp"

#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include <Slic3r/Biz/I18N/I18N.hpp>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigRowItem::ConfigRowItem(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::IConfigBoxSetter& cb_setter,
    size_t cbi_index,
    std::optional<std::string> force_label
) :
    Biz::DataObserver<Domain::ConfigItem>(index, data),
    m_cb_setter(cb_setter),
    m_cbi_index(cbi_index),
    m_force_label(force_label)
{
    set_fill(m_theme->color_imgui(Platform::Color::Transparent));
    set_border_width(2);
    set_border_color(m_theme->color_imgui(Platform::Color::Transparent));
    set_gap(5.f);

    m_left_side = emplace_back<Item>();

    m_label = m_left_side->emplace_back<Text>(m_force_label.value_or(Biz::_u8(data.def().label)));
    m_label->set_height(40);
    m_label->set_wrap_mode(Text::WrapMode::WrapElide);
    m_label->set_align({AlignH::Left, AlignV::Center});

    m_sidetext = emplace_back<Text>(Biz::_u8(m_state->def().sidetext));
    m_sidetext->set_self_align(YGAlignCenter);
    m_sidetext->set_flex_grow(1.f);
    m_sidetext->set_wrap_mode(Text::WrapMode::WrapElide);

    m_label->set_width(m_force_label.has_value() ? 90 : 175);
    m_left_side->set_max_width(175);

    on_data_update();
}

void ConfigRowItem::set_label_text_color(const ImColor& color)
{
    m_label->set_text_color(color);
}

void ConfigRowItem::on_data_update()
{
    if (m_created_gui_type != m_state->def().gui_type) {
        // We got a new ConfigItem assigned with different GuiType
        m_created_gui_type = m_state->def().gui_type;

        if (m_input) {
            remove(m_input);
        }

        m_control = ConfigItemControl::config_item_control_factory(
            this,
            1,
            m_index,
            *m_state,
            m_cb_setter,
            m_cbi_index
        );

        m_input = dynamic_cast<Yoga::Item*>(m_control);
        ASSERT(m_input, "ConfigItem needs to derive from Yoga::Item");

        if (m_state->def().gui_type == Slic3r::Domain::ConfigItemDef::GUIType::spinbox) {
            m_config_item_spin_box = dynamic_cast<ConfigItemSpinBox*>(m_input);
        }
    }

    if (m_input && (!m_last_full_width || m_last_full_width.value() != m_state->def().full_width)) {
        m_input->set_flex_grow(m_state->def().full_width);
        if (m_state->def().full_width) {
            set_orientation(Orientation::Vertical);
            set_align_items(YGAlign::YGAlignStretch);
        } else {
            set_orientation(Orientation::Horizontal);
            set_align_items(YGAlign::YGAlignCenter);
        }
        m_last_full_width = m_state->def().full_width;
    }

    if (m_state->def().type != m_created_value_type) {
        // We got a new ConfigItem assigned with different value type
        m_created_value_type = m_state->def().type;
        if (*m_state->def().type == typeid(std::optional<int>)) {
            m_toggle_enable = m_left_side->emplace_back<ToggleButton>();
            m_toggle_enable->set_margin(Margins(0, 0, 5, 0));

            std::optional<int> value = m_state->value().get<std::optional<int>>();
            m_toggle_enable->set_checked(value.has_value());
            m_toggle_enable->callbacks().action = [this]()
            {
                // We are using action to make sure this callbacks comes from user
                std::optional<int> value;
                if (m_toggle_enable->checked()) {
                    value = m_config_item_spin_box->value();
                }

                m_cb_setter.set_item_value(*m_state, Domain::ConfigValue{value}, m_cbi_index);
            };
        } else {
            if (m_toggle_enable) {
                m_left_side->remove_later(m_toggle_enable);
                m_toggle_enable = nullptr;
            }
        }
    }

    m_label->set_text(m_force_label.value_or(Biz::_u8(m_state->def().label)));
    m_sidetext->set_text(Biz::_u8(m_state->def().sidetext));
    m_sidetext->set_min_width(m_sidetext->text().empty() ? 0 : 30);

    if (*m_state->def().type == typeid(std::optional<int>)) {
        std::optional<int> value = m_state->value().get<std::optional<int>>();
        m_toggle_enable->set_checked(value.has_value());
    }

    m_control->set_state(*m_state);
}

void ConfigRowItem::navigate_to_item(const Domain::ConfigItem* config_item)
{
    if (m_state == config_item) {
        set_border_color(m_theme->color_imgui(Platform::Color::AccentPrimary));
    } else {
        set_border_color(m_theme->color_imgui(Platform::Color::Transparent));
    }
}

void ConfigRowItem::clear_navigation()
{
    set_border_color(IM_COL32_BLACK_TRANS);
}

} // namespace Slic3r::App
