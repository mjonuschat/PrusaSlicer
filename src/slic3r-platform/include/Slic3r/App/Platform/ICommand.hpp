#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "Slic3r/App/Platform/KeyboardShortcut.hpp"

namespace Slic3r::App::Platform {

using KeyboardShortcuts = std::vector<KeyboardShortcut>;

class ICommand
{
public:
    virtual ~ICommand() = default;

    virtual const char* name() const = 0;
    virtual void execute() const     = 0;

    virtual const std::optional<KeyboardShortcuts> keyboard_shortcuts() const
    {
        return std::nullopt;
    }

    virtual bool enabled() const
    {
        return true;
    }

    std::string keyboard_shortcut_string(KeyboardShortcut::Translator translator) const
    {
        return keyboard_shortcuts() ? keyboard_shortcuts().value().front().to_string(translator) :
                                     std::string();
    }

    std::vector<std::string> keyboard_shortcut_accel_string() const
    {
        std::vector<std::string> ret;
        if (keyboard_shortcuts()) {
            KeyboardShortcuts kb_shortcuts = keyboard_shortcuts().value();
            for (const KeyboardShortcut& kb_shortcut : kb_shortcuts) {
                ret.emplace_back(kb_shortcut.to_accel_table_string());
            }
        }
        return ret;
    }
};

struct FuncCommandExtraOpts
{
    std::optional<KeyboardShortcuts> keyboard_shortcuts = std::nullopt;
    std::function<bool()> enabled                      = nullptr;
};

class FuncCommand final : public ICommand
{
public:
    FuncCommand(
        const std::string& name,
        std::function<void()> execute,
        FuncCommandExtraOpts extra_opts = FuncCommandExtraOpts{}
    ) :
        m_name(name),
        m_execute(std::move(execute)),
        m_extra_opts(extra_opts)
    {}

    const char* name() const override
    {
        return m_name.c_str();
    }

    void execute() const override
    {
        m_execute();
    }

    const std::optional<KeyboardShortcuts> keyboard_shortcuts() const override
    {
        return m_extra_opts.keyboard_shortcuts;
    }

    bool enabled() const override
    {
        return m_extra_opts.enabled ? m_extra_opts.enabled() : true;
    }

private:
    std::string m_name;

    std::function<void()> m_execute;
    FuncCommandExtraOpts m_extra_opts;
};

} // namespace Slic3r::App::Platform
