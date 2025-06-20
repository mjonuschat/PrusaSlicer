#pragma once

#include "Slic3r/App/Browser/AbstractBrowserLogic.hpp"
#include "Slic3r/Assert.hpp"
#include <memory>
#include <boost/filesystem/path.hpp>

namespace Slic3r::Biz {
class ProjectInteractor;
}

namespace Slic3r::App {


enum class FileDialogType
{
    Open,
    Save
};

class IDialogManager {
public:
    using FileCallback = std::function<void(bool result, const boost::filesystem::path& file_path)>;
    using YesNoCallback = std::function<void(bool answer)>;

    IDialogManager() = default;
    virtual ~IDialogManager() = default;

    virtual void show_file_dialog(
        FileDialogType dialog_type,
        const std::string& dialog_title, 
        const boost::filesystem::path& default_folder,  
        const std::string& default_file_name, 
        const std::string& wildcards,
        const FileCallback& callback
    ) = 0;

    virtual void show_webview_dialog(std::unique_ptr<Browser::AbstractBrowserLogic>&& logic, Slic3r::Biz::ProjectInteractor* project_interactor) = 0;
    virtual void show_yesno_dialog(const std::string& title, const std::string& text, const YesNoCallback& callback) = 0;
};

/**
  * Singleton class to manage the dialog manager implementation.
  * Creation of any modal dialog from Slic3r::App should be done by calling DialogManagerProvider.get() and implemented methods in IDialogManager.
  */
class DialogManagerProvider {
public:
    static DialogManagerProvider& instance()
    {
        static DialogManagerProvider instance;
        return instance;
    }

    void set_dialog_manager_implementation(std::unique_ptr<IDialogManager>&& manager) 
    {
        ASSERT(manager);
        m_manager = std::move(manager);
    }

    IDialogManager& get()
    {
        ASSERT(m_manager != nullptr);
        return *m_manager;
    }

private:
    std::unique_ptr<IDialogManager> m_manager{};
};

} // namespace Slic3r::App