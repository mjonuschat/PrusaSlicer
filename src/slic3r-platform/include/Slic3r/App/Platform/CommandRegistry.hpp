#pragma once

#include <unordered_map>
#include <memory>

#include "Slic3r/App/Platform/KeyboardEvent.hpp"
#include "Slic3r/App/Platform/ICommand.hpp"

namespace Slic3r::App::Platform {

/**
 * @brief Registry for commands user can invoke.
 */
class CommandRegistry
{
public:
    /**
     * @brief Register single command
     * @param command Pointer to command imeplementaiton
     */
    CommandRegistry& register_command(std::unique_ptr<ICommand> command);

    /**
     * @brief Process keyboard event and eventually execute related command.
     * @param e Keyboard generated event to process.
     * @return `true` if the event was consumed (command invoked), otherwise `false`.
     */
    bool process_keyboard_event(const KeyboardEvent& e);

private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> m_commands_by_id;
};

} // Slic3r::App::Platform
