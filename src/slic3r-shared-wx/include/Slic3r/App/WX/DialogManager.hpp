#pragma once 

#include <Slic3r/App/IDialogManager.hpp>

namespace Slic3r::App::WX {

class DialogManager : public IDialogManager{
public:
    DialogManager() = default;

    void show_file_dialog(
        FileDialogType dialog_type,
        const std::string& dialog_title, 
        const boost::filesystem::path& default_folder,  
        const std::string& default_file_name, 
        const std::string& wildcards,
        const std::function<void(bool result, const boost::filesystem::path& file_path)>& callback
    ) override;

    void show_webview_dialog(std::unique_ptr<App::Browser::AbstractBrowserLogic>&& logic, Slic3r::Biz::ProjectInteractor* project_interactor) override;
};

} //namespace Slic3r::App::WX