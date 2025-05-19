#pragma once 

#include <Slic3r/App/IDialogManager.hpp>

namespace Slic3r::App::WX {

class DialogManager : public IDialogManager{
public:
    DialogManager() = default;

    void show_save_file_dialog(
        const std::string& dialog_title, 
        const boost::filesystem::path& default_folder,  
        const std::string& default_file_name, 
        const std::string& wildcards,
        const std::function<void(bool result, const boost::filesystem::path& file_path)>& callback
    ) override;
};

} //namespace Slic3r::App::WX