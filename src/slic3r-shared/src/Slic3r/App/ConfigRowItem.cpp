///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ConfigRowItem.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Config/ConfigItemSpinBox.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigRowItem::ConfigRowItem(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor,
    bool small
) :
    Biz::DataObserver<Domain::ConfigItem>(index, data),
    m_preset_interactor(preset_interactor)
{
    set_flex_shrink(0);

    Item* left_side = emplace_back<Item>();

    if (*m_state->def().type == typeid(std::optional<int>)) {
        m_toggle_enable = left_side->emplace_back<ToggleButton>();
        m_toggle_enable->set_margin(Margins(0, 0, 5, 0));

        std::optional<int> value = m_state->value().get<std::optional<int>>();
        m_toggle_enable->set_checked(value.has_value());
        m_toggle_enable->callbacks().action = [this]() {
            // We are using action to make sure this callbacks comes from user
            std::optional<int> value;
            if (m_toggle_enable->checked()) {
                value = m_config_item_spin_box->value();
            }

            m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{value});
        };
    }

    m_label = left_side->emplace_back<Text>(data.def().label);

    m_control = ConfigItemControl::config_item_control_factory(this, index, data, m_preset_interactor);

    m_input = dynamic_cast<Yoga::Item*>(m_control);
    ASSERT(m_input, "ConfigItem needs to derive from Yoga::Item");

    if (data.def().gui_type == Slic3r::Domain::ConfigItemDef::GUIType::spinbox) {
        m_config_item_spin_box = dynamic_cast<ConfigItemSpinBox*>(m_input);
    }

    m_sidetext = emplace_back<Text>(data.def().sidetext);
    m_sidetext->set_self_align(YGAlignCenter);

    if (small) {
        set_gap(5);
        set_width(175);
        m_input->set_min_size({50, YGUndefined});

        left_side->set_flex_grow(1);
        m_label->set_self_align(YGAlignCenter);
    } else {
        m_label->set_flex_grow(1);
        m_label->set_height(40);
        m_label->set_wrap_mode(Text::WrapMode::WrapElide);
        m_label->set_align({AlignH::Left, AlignV::Center});
        m_label->set_padding(Paddings(0, 0, 5, 0));

        if (data.def().full_width) {
            m_input->set_flex_grow(1);
            set_orientation(Orientation::Vertical);
        } else {
            set_align_items(YGAlign::YGAlignCenter);
        }

        left_side->set_width(175);
        left_side->set_max_size({175, YGUndefined});
    }
}

void ConfigRowItem::on_data_update()
{
    m_label->set_text(m_state->def().label);
    m_sidetext->set_text(m_state->def().sidetext);

    if (*m_state->def().type == typeid(std::optional<int>)) {
        std::optional<int> value = m_state->value().get<std::optional<int>>();
        m_toggle_enable->set_checked(value.has_value());
    }

    if (m_control) {
        m_control->set_state(*m_state);
    }
}

} // namespace Slic3r::App
