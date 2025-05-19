#pragma once

#include "Slic3r/Assert.hpp"
#include <memory>
#include <boost/filesystem/path.hpp>

namespace Slic3r::App {

class IDialogManager {
public:
    IDialogManager() = default;
    virtual ~IDialogManager() = default;

    virtual void show_save_file_dialog(
        const std::string& dialog_title, 
        const boost::filesystem::path& default_folder,  
        const std::string& default_file_name, 
        const std::string& wildcards,
        const std::function<void(bool result, const boost::filesystem::path& file_path)>& callback
    ) = 0;
};

/**
  * Singleton class to manage the dialog manager implementation.
  * Creation of any modal dialog from Slic3r::App should be done by calling DialogManagerSingleton.get() and implemented methods in IDialogManager.
  */
class DialogManagerSingleton {
public:
    static DialogManagerSingleton& instance()
    {
        static DialogManagerSingleton instance;
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
    std::unique_ptr<IDialogManager> m_manager {};
};

} // namespace Slic3r::App