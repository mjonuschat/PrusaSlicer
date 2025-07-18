#include "Slic3r/App/AppServices.hpp"

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
} // namespace Slic3r::App
