#include "Slic3r/App/WX/DialogManager.hpp"

#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"

#include <wx/filedlg.h>
#include <wx/string.h>

namespace Slic3r::App::WX {

void DialogManager::show_save_file_dialog(
        const std::string& dialog_title, 
        const boost::filesystem::path& default_folder,  
        const std::string& default_file_name, 
        const std::string& wildcards,
        const std::function<void(bool result, const boost::filesystem::path& file_path)>& callback
    )
{
    ASSERT(callback);

    wxFileDialog dlg(nullptr,
        from_u8(dialog_title),
        from_u8(default_folder.string()),
        from_u8(default_file_name),
        from_u8(wildcards),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );

    if (dlg.ShowModal() != wxID_OK) {
        callback(false, {});
       return;
    }
    wxString out_path = dlg.GetPath();
    callback(true, into_path(out_path));
}

}