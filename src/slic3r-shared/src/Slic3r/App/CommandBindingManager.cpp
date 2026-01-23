#include "Slic3r/App/CommandBindingManager.hpp"

#include "Slic3r/App/UIItemCommand.hpp"
#include "Slic3r/App/Yoga/AbstractButton.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

namespace Slic3r::App {

static auto translator = [](const std::string& s) { return Biz::_u8(s); };

void
CommandBindingManager::bind_menu_item(const UIItemCommand* command, Yoga::AbstractButton* ui_item)
{
    ui_item->callbacks().action = [this, command]() { command->execute(); };

    ui_item->callbacks().checked_changed = [command](bool checked)
    { command->checked_changed(checked); };

    ui_item->set_shortcut(command->keyboard_shortcut_string(translator));

    bind(command->name(), ui_item);
}

void CommandBindingManager::bind_tb_item(const char* command_name, Yoga::AbstractButton* ui_item)
{
    ui_item->callbacks().action = [this, command_name]()
    { m_command_registry.command(command_name).execute(); };

    ui_item->set_shortcut(m_command_registry.command(command_name)
                              .keyboard_shortcut_string(translator));

    if (const UIItemCommand* ui_command =
            dynamic_cast<UIItemCommand*>(&m_command_registry.command(command_name)))
    {
        ui_item->callbacks().checked_changed = [ui_command](bool checked)
        { ui_command->checked_changed(checked); };
    }

    bind(command_name, ui_item);
}

void CommandBindingManager::update_ui_items()
{
    for (auto& [command_name, items] : m_ui_items) {
        const bool enabled = m_command_registry.command(command_name.c_str()).enabled();
        for (Yoga::AbstractButton* ui_item : items) {
            ui_item->set_visible(enabled);
        }
    }
}

void CommandBindingManager::bind(const char* command_name, Yoga::AbstractButton* ui_item)
{
    if (!m_ui_items.contains(command_name))
        m_ui_items[command_name].reserve(3); // let it be max 3 ui_item per command

    m_ui_items.at(command_name).emplace_back(ui_item);
}

} // namespace Slic3r::App
