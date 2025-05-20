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
};

} //namespace Slic3r::App::WX