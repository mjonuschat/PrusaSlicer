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
        const auto shortcut = cmd.second->keyboard_shortcut();
        if (!shortcut.has_value() || !cmd.second->enabled()) {
            continue;
        }
        if (e.key_modifiers() == shortcut.value().modifiers && e.code() == shortcut.value().key) {
            cmd.second->execute();
            return true;
        }
    }

    return false;
}

const ICommand& CommandRegistry::command(const char* name) const
{
    ASSERT(m_commands_by_id.contains(name), "Non-existed command");
    return *m_commands_by_id.at(name).get();
}

bool CommandRegistry::has_command(const char* name) const
{
    return m_commands_by_id.contains(name);
}

} // namespace Slic3r::App::Platform
