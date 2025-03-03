#pragma once

#include <vector>
#include <unordered_map>

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
     * @param takes_over_ownership If `false` user is responsible for deallocation of the command
     * (at app quit), otherwise the CommandRegistry takes over an ownership of the object and will
     * deallocate it on its own.
     */
    CommandRegistry& register_command(ICommand* command, bool takes_over_ownership=false);

    /**
     * @brief Process keyboard event and eventually execute related command.
     * @param e Keyboard generated event to process.
     * @return `true` if the event was consumed (command invoked), otherwise `false`.
     */
    bool process_keyboard_event(const KeyboardEvent& e);

private:
    struct CommandInfo final
    {
        ICommand* command{nullptr};
        bool owned{false};

        CommandInfo(ICommand* command, bool owned) : command(command), owned(owned) {}
        CommandInfo(CommandInfo&& other) noexcept : command(other.command), owned(other.owned)
        {
            other.command = nullptr;
        }

        ~CommandInfo()
        {
            if (owned && command != nullptr)
                delete command;
        }
    };

    std::vector<CommandInfo> m_commands;
    std::unordered_map<std::string, ICommand*> m_commands_by_id;
};

} // Slic3r::App::Platform
