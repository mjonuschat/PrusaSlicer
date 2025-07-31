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
        const FileCallback& callback
    ) override;

    void show_webview_dialog(std::unique_ptr<App::Browser::AbstractBrowserLogic>&& logic, Slic3r::Biz::ProjectInteractor* project_interactor) override;
    void show_yesno_dialog(const std::string& title, const std::string& text, const YesNoCallback& callback) override;
    void show_rich_yesno_dialog(
        const std::string& title,
        const std::string& text,
        const std::string& check_text,
        const YesNoCallback& callback,
        const CheckBoxCheckedCallback& checked_callback
    ) override;
    void show_info_dialog(const std::string& text, const std::string& title = std::string()) override;
    void show_warning_dialog(const std::string& text, const std::string& title = std::string()) override;
    void show_error_dialog(const std::string& text, const std::string& title = std::string()) override;
};

} //namespace Slic3r::App::WX