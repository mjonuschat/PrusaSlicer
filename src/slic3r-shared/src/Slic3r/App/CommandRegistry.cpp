#include "Slic3r/App/CommandRegistry.hpp"
#include "Slic3r/Assert.hpp"


namespace Slic3r::App {

CommandRegistry& CommandRegistry::register_command(ICommand* command, bool takes_over_ownership)
{
    ASSERT(command);
    const char* name = ASSERT_VAL(command->name());
    ASSERT(m_commands_by_id.count(name) == 0, "Command with same name already registered");
    m_commands.emplace_back(command, takes_over_ownership);
    m_commands_by_id[name] = command;
    return *this;
}

bool CommandRegistry::process_keyboard_event(const Platform::KeyboardEvent& e)
{
    if (e.type() != Platform::KeyboardEvent::Type::KeyDown)
        return false;

    for (auto& cmd : m_commands) {
        const auto* shortcut = cmd.command->keyboard_shortcut();
        if (shortcut == nullptr || !cmd.command->enabled())
            continue;
        if (e.key_modifiers() == shortcut->modifiers && e.code() == shortcut->key) {
            cmd.command->execute();
            return true;
        }
    }

    return false;
}

}
