///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemExtruderSelection.hpp"

#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include "Slic3r/App/Yoga/ComboBox.hpp"

namespace Slic3r::App {

ConfigItemExtruderSelection::ConfigItemExtruderSelection(
    size_t index,
    const Domain::ConfigItem& config_item,
    Biz::IConfigBoxSetter& cbi_container,
    size_t cbi_index
) :
    ConfigItemControl(index, config_item),
    ComboBox("ConfigItemCombo"),
    m_preset_changed_scope(project_interactor()->preset_interactor(), *this),
    m_cbi_container(cbi_container),
    m_cbi_index(cbi_index)
{
    set_width(150);

    update_size();
    on_data_update();

    m_tooltip->set_text(tooltip_text());
    m_tooltip->content_item()->set_width(350);
    m_tooltip->set_text_wrap(true);

    callbacks().selection_changed = [this](int selected)
    {
        if (current_index() <= project_interactor()->preset_interactor().tool_items().size()) {
            m_cbi_container
                .set_item_value(*m_state, Domain::ConfigValue{current_index()}, m_cbi_index);
            update_size();
        }
    };
}

void ConfigItemExtruderSelection::on_preset_selection_changed(
    Domain::SelectionId project_id,
    Domain::SelectionId config_container_id,
    Biz::Preset::PresetItemType type
)
{
    if (project_interactor()->selected_project_id() == project_id
        && project_interactor()->selected_config_container_id() == config_container_id
        && type == Biz::Preset::PresetItemType::PrinterPreset)
    {
        update_size();
    }
}

void ConfigItemExtruderSelection::update_size()
{
    const size_t slot_count = project_interactor()
                                  ->preset_interactor()
                                  .selected_printer_preset()
                                  .hw_config.material_slot_count();
    if (slot_count != m_items.size()) {
        std::vector<std::string> new_items;
        new_items.reserve(slot_count + 1);
        for (size_t i = 0; i <= slot_count; ++i) {
            if (i == 0) {
                new_items.push_back(Biz::_u8L("Default"));
            } else {
                new_items.push_back(std::to_string(i));
            }
        }

        if (!mixed() && m_state->value().get<int>() > slot_count) {
            new_items.push_back(std::to_string(m_state->value().get<int>()));
            set_items(new_items);
            set_current_index(slot_count + 1);
        } else {
            set_items(new_items);
        }
    }
}

void ConfigItemExtruderSelection::on_data_update()
{
    if (mixed()) {
        set_override_label(Biz::_u8L("Mixed"));
        set_label_font_type(Render::ImguiFontType::Italic);
        return;
    }

    set_override_label(std::string());
    set_label_font_type(Render::ImguiFontType::Regular);
    if (!overriden().value_or(true)) {
        update_value(*m_cbi_container.get_override_original_value(*m_state, location_index()));
    } else {
        update_value(m_state->value());
    }
}

void ConfigItemExtruderSelection::update_value(const Domain::ConfigValue& value)
{
    set_current_index(value.get<int>());
}

} // namespace Slic3r::App
