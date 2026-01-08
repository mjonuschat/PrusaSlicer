#pragma once

#include "Slic3r/App/Platform/CommandRegistry.hpp"

namespace Slic3r::App {

namespace Yoga {
class AbstractButton;
} // namespace Yoga

class UIItemCommand;

class CommandBindingManager
{
public:
    using UIItemsPerCommandMap =
        std::unordered_map<std::string, std::vector<Yoga::AbstractButton*>>;

    CommandBindingManager(Platform::CommandRegistry& command_registry) :
        m_command_registry(command_registry)
    {}

    void bind_menu_item(const UIItemCommand* command, Yoga::AbstractButton* ui_item);

    void bind_tb_item(const char* command_name, Yoga::AbstractButton* ui_item);

    void update_ui_items();

    Platform::CommandRegistry& command_registry()
    {
        return m_command_registry;
    }

private:
    void bind(const char* command_name, Yoga::AbstractButton* ui_item);

    Platform::CommandRegistry& m_command_registry;

    UIItemsPerCommandMap m_ui_items;
};

} // namespace Slic3r::App
