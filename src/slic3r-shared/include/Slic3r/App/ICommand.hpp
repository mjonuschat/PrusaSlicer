#pragma once

#include <functional>
#include <optional>
#include <string>
#include "Slic3r/App/KeyboardShortcut.hpp"

namespace Slic3r::App {
class ICommand
{
public:
    virtual ~ICommand() = default;

    virtual const char* name() const = 0;
    virtual void execute() = 0;

    virtual const KeyboardShortcut* keyboard_shortcut() const { return nullptr; }
    virtual bool enabled() const { return true; }
};

class FuncCommand final : public ICommand
{
public:
    FuncCommand(
        const char* name,
        std::function<void()> execute,
        std::function<bool()> enabled = nullptr,
        std::optional<KeyboardShortcut> keyboard_shortcut = std::nullopt
    )
        : m_name(name)
        , m_execute(std::move(execute))
        , m_enabled(std::move(enabled))
        , m_keyboard_shortcut(keyboard_shortcut)
    {}

    const char* name() const override { return m_name.c_str(); }

    void execute() override { m_execute(); }

    const KeyboardShortcut* keyboard_shortcut() const override
    {
        return m_keyboard_shortcut.has_value() ? &*m_keyboard_shortcut : nullptr;
    }

    bool enabled() const override { return m_enabled ? m_enabled() : true; }

private:
    std::string m_name;

    std::function<void()> m_execute;
    std::function<bool()> m_enabled;
    std::optional<KeyboardShortcut> m_keyboard_shortcut;
};

} // namespace Slic3r::App
