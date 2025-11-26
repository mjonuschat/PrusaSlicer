#include "Slic3r/App/AppServices.hpp"

#include "Slic3r/App/PopNotification/PopNotificationCenter.hpp"
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/Platform/IFileExplorerHandler.hpp"
#include "Slic3r/App/AppConfig.hpp"
#include "Slic3r/App/AppConfigInteractor.hpp"

namespace Slic3r::App {

AppServices& AppServices::instance()
{
    static AppServices instance;
    return instance;
}

AppServices::~AppServices() = default;

void AppServices::set_pop_notification_center(std::unique_ptr<PopNotification::PopNotificationCenter>&& ptr)
{
    m_pop_notification_center = std::move(ptr);
}

void AppServices::set_dialog_manager(std::unique_ptr<IDialogManager>&& manager)
{
    m_dialog_manager = std::move(manager);
}

void AppServices::set_file_explorer_handler(std::unique_ptr<Platform::IFileExplorerHandler>&& handler)
{
    m_file_explorer_handler = std::move(handler);
}

void AppServices::set_app_config(std::unique_ptr<AppConfig>&& app_config)
{
    m_app_config = std::move(app_config);
    ASSERT(m_app_config != nullptr);
    m_app_config_interactor = std::make_unique<AppConfigInteractor>(m_app_config->get_config_box_ptr());
}

PopNotification::PopNotificationCenter& AppServices::pop_notification_center() const
{
    ASSERT(m_pop_notification_center != nullptr);
    return *m_pop_notification_center;
}

IDialogManager& AppServices::dialog_manager() const
{
    ASSERT(m_dialog_manager != nullptr);
    return *m_dialog_manager;
}

Platform::IFileExplorerHandler& AppServices::file_explorer_handler() const
{
    ASSERT(m_file_explorer_handler != nullptr);
    return *m_file_explorer_handler;
}

AppConfig& AppServices::app_config() const
{
    ASSERT(m_app_config != nullptr);
    return *m_app_config;
}

AppConfigInteractor& AppServices::app_config_intractor() const
{
    ASSERT(m_app_config_interactor != nullptr);
    return *m_app_config_interactor;
}

} // namespace Slic3r::App
