#pragma once

#include "Slic3r/Assert.hpp"

#include "Slic3r/App/PopNotification/PopNotificationCenter.hpp"
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/Platform/IFileExplorerHandler.hpp"

#include <memory>

namespace Slic3r::App {

class AppServices
{
public:
    static AppServices& instance();
    ~AppServices();

    void set_pop_notification_center(std::unique_ptr<PopNotification::PopNotificationCenter>&& ptr);
    void set_dialog_manager(std::unique_ptr<IDialogManager>&& manager);
    void set_file_explorer_handler(std::unique_ptr<Platform::IFileExplorerHandler>&& handler);

    PopNotification::PopNotificationCenter& pop_notification_center() const
    {
        ASSERT(m_pop_notification_center != nullptr);
        return *m_pop_notification_center;
    }

    IDialogManager& dialog_manager()
    {
        ASSERT(m_dialog_manager != nullptr);
        return *m_dialog_manager;
    }

    Platform::IFileExplorerHandler& file_explorer_handler()
    {
        ASSERT(m_file_explorer_handler != nullptr);
        return *m_file_explorer_handler;
    }

private:
    std::unique_ptr<PopNotification::PopNotificationCenter> m_pop_notification_center;
    std::unique_ptr<IDialogManager> m_dialog_manager{};
    std::unique_ptr<Platform::IFileExplorerHandler> m_file_explorer_handler;
};

} // namespace Slic3r::App
