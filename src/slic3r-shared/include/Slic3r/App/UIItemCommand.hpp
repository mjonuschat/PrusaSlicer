#pragma once

#include "Slic3r/App/Platform/ICommand.hpp"

namespace Slic3r::App {

struct UIItemCommandExtraOpts
{
    std::optional<std::vector<Platform::KeyboardShortcut>> keyboard_shortcuts = std::nullopt;
    std::function<bool()> enabled                               = nullptr;
    std::function<bool()> visible                               = nullptr;
    std::function<bool()> checked                               = nullptr;
    std::function<void(bool)> checked_changed                   = nullptr;
};

class UIItemCommand final : public Platform::ICommand
{
public:

    UIItemCommand(
        const char* name,
        std::function<void()> execute,
        UIItemCommandExtraOpts extra_opts = UIItemCommandExtraOpts{}
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

    const std::optional<std::vector<Platform::KeyboardShortcut>> keyboard_shortcuts() const override
    {
        return m_extra_opts.keyboard_shortcuts;
    }

    bool enabled() const override
    {
        return m_extra_opts.enabled ? m_extra_opts.enabled() : true;
    }

    bool visible() const
    {
        return m_extra_opts.visible ? m_extra_opts.visible() : true;
    }

    bool checked() const
    {
        return m_extra_opts.checked ? m_extra_opts.checked() : false;
    }

    void checked_changed(bool checked) const
    {
        if (m_extra_opts.checked_changed) {
            m_extra_opts.checked_changed(checked);
        }
    }

private:
    std::string m_name;

    std::function<void()> m_execute;
    UIItemCommandExtraOpts m_extra_opts;
};

} // namespace Slic3r::App
