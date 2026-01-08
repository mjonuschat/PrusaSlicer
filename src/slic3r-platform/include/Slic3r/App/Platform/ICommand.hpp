#pragma once

#include <functional>
#include <optional>
#include <string>
#include "Slic3r/App/Platform/KeyboardShortcut.hpp"

namespace Slic3r::App::Platform {
class ICommand
{
public:
    virtual ~ICommand() = default;

    virtual const char* name() const = 0;
    virtual void execute() const     = 0;

    virtual const std::optional<KeyboardShortcut> keyboard_shortcut() const
    {
        return std::nullopt;
    }

    virtual bool enabled() const
    {
        return true;
    }

    const std::string keyboard_shortcut_string() const
    {
        return keyboard_shortcut() ? keyboard_shortcut().value().to_string() : std::string();
    }
};

struct FuncCommandExtraOpts
{
    std::optional<Platform::KeyboardShortcut> keyboard_shortcut = std::nullopt;
    std::function<bool()> enabled = nullptr;
};

class FuncCommand final : public ICommand
{
public:
    FuncCommand(
        const char* name,
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

    const std::optional<KeyboardShortcut> keyboard_shortcut() const override
    {
        return m_extra_opts.keyboard_shortcut;
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
