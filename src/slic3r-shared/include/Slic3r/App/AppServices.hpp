#pragma once

#include "Slic3r/Assert.hpp"
#include <memory>

namespace Slic3r::App::PopNotification {
class PopNotificationCenter;
}

namespace Slic3r::App::Platform {
class IFileExplorerHandler;
}

namespace Slic3r::App {

class AppConfig;
class IDialogManager;
class AppConfigInteractor;

class AppServices
{
public:
    static AppServices& instance();
    ~AppServices();

    void set_pop_notification_center(std::unique_ptr<PopNotification::PopNotificationCenter>&& ptr);
    void set_dialog_manager(std::unique_ptr<IDialogManager>&& manager);
    void set_file_explorer_handler(std::unique_ptr<Platform::IFileExplorerHandler>&& handler);
    void set_app_config(std::unique_ptr<AppConfig>&& app_config);

    PopNotification::PopNotificationCenter& pop_notification_center() const;
    IDialogManager& dialog_manager() const;
    Platform::IFileExplorerHandler& file_explorer_handler() const;
    AppConfig& app_config() const;
    AppConfigInteractor& app_config_interactor() const;

private:
    std::unique_ptr<PopNotification::PopNotificationCenter> m_pop_notification_center;
    std::unique_ptr<IDialogManager> m_dialog_manager{};
    std::unique_ptr<Platform::IFileExplorerHandler> m_file_explorer_handler;
    std::unique_ptr<AppConfig> m_app_config;
    std::unique_ptr<AppConfigInteractor> m_app_config_interactor;
};

} // namespace Slic3r::App
