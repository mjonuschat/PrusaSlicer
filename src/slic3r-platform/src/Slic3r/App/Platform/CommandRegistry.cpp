#include "Slic3r/App/Platform/CommandRegistry.hpp"
#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Platform {

CommandRegistry& CommandRegistry::register_command(std::unique_ptr<ICommand> command)
{
    ASSERT(command);
    const char* name = ASSERT_VAL(command->name());
    ASSERT(m_commands_by_id.count(name) == 0, "Command with same name already registered");
    m_commands_by_id[name] = std::move(command);
    return *this;
}

bool CommandRegistry::process_keyboard_event(const KeyboardEvent& e)
{
    if (e.type() != KeyboardEvent::Type::KeyDown)
        return false;

    for (const auto& cmd : std::as_const(m_commands_by_id)) {
        const auto* shortcut = cmd.second->keyboard_shortcut();
        if (shortcut == nullptr || !cmd.second->enabled()) {
            continue;
        }
        if (e.key_modifiers() == shortcut->modifiers && e.code() == shortcut->key) {
            cmd.second->execute();
            return true;
        }
    }

    return false;
}

} // Slic3r::App::Platform
