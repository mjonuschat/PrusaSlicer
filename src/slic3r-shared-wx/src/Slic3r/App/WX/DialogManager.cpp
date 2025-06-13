#include "Slic3r/App/WX/DialogManager.hpp"

#include "Slic3r/App/WX/WebView/WebViewDialog.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <wx/filedlg.h>
#include <wx/string.h>

namespace Slic3r::App::WX {

void DialogManager::show_file_dialog(
        FileDialogType dialog_type,
        const std::string& dialog_title, 
        const boost::filesystem::path& default_folder,  
        const std::string& default_file_name, 
        const std::string& wildcards,
        const std::function<void(bool result, const boost::filesystem::path& file_path)>& callback
    )
{
    ASSERT(callback);

    int flags = 0;

    switch (dialog_type) {
    case FileDialogType::Open:
        flags |= wxFD_OPEN;
        break;

        case FileDialogType::Save:
        flags |= wxFD_SAVE | wxFD_OVERWRITE_PROMPT;
        break;
    }

    wxFileDialog dlg(nullptr,
        from_u8(dialog_title),
        from_u8(default_folder.string()),
        from_u8(default_file_name),
        from_u8(wildcards),
        flags
    );

    if (dlg.ShowModal() != wxID_OK) {
        callback(false, {});
       return;
    }
    wxString out_path = dlg.GetPath();
    callback(true, into_path(out_path));
}

void DialogManager::show_webview_dialog(std::unique_ptr<App::Browser::AbstractBrowserLogic>&& logic, Biz::ProjectInteractor* project_interactor)
{
    WebView::WebViewDialog dlg(std::move(logic));
    project_interactor->user_account_interactor().add_listener<Biz::UserAccount::IUserAccountListener>(&dlg);
    dlg.ShowModal();
    project_interactor->user_account_interactor().remove_listener<Biz::UserAccount::IUserAccountListener>(&dlg);
}

}