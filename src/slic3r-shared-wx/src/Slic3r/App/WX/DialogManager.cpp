#include "Slic3r/App/WX/DialogManager.hpp"

#include "Slic3r/App/WX/WebView/WebViewDialog.hpp"
#include "Slic3r/App/WX/MsgDialog.hpp"
#include "Slic3r/App/I18N/I18N.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include <Slic3r/App/WX/I18N.hpp>

#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/string.h>

namespace Slic3r::App::WX {

void DialogManager::show_file_dialog(
        FileDialogType dialog_type,
        const std::string& dialog_title, 
        const boost::filesystem::path& default_folder,  
        const std::string& default_file_name, 
        const std::string& wildcards,
        const FileCallback& callback
    )
{
    ASSERT(callback);

    int flags = 0;

    switch (dialog_type) {
    case FileDialogType::Open:
        flags |= wxFD_OPEN;
        break;
    case FileDialogType::OpenMultiple:
        flags |= wxFD_OPEN | wxFD_MULTIPLE;
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

    std::vector<boost::filesystem::path> out_paths;
    if (dialog_type == FileDialogType::OpenMultiple) {
        wxArrayString paths;
        dlg.GetPaths(paths);

        for (const wxString& path : paths) {
            out_paths.emplace_back(into_path(path));
        }
    } else {
        out_paths.emplace_back(into_path(dlg.GetPath()));
    }

    callback(true, out_paths);
}

void DialogManager::show_webview_dialog(std::unique_ptr<App::Browser::AbstractBrowserLogic>&& logic, Biz::ProjectInteractor* project_interactor)
{
    WebView::WebViewDialog dlg(std::move(logic));
    project_interactor->user_account_interactor().add_listener<Biz::UserAccount::IUserAccountListener>(&dlg);
    dlg.ShowModal();
    project_interactor->user_account_interactor().remove_listener<Biz::UserAccount::IUserAccountListener>(&dlg);
}

void DialogManager::show_yesno_dialog(const std::string& title, const std::string& text, const YesNoCallback& callback)
{
    wxMessageDialog dlg(nullptr, from_u8(text), from_u8(title), wxYES_NO);
    if (dlg.ShowModal() == wxID_YES)
        callback(true);
    else
        callback(false);
}

void DialogManager::show_rich_yesno_dialog(
    const std::string& title,
    const std::string& text,
    const std::string& check_text,
    const YesNoCallback& callback,
    const CheckBoxCheckedCallback& cbc_callback
)
{
    RichMessageDialog dlg(nullptr, from_u8(text), from_u8(title), wxICON_QUESTION | wxYES_NO);
    dlg.ShowCheckBox(from_u8(check_text));
    if (dlg.ShowModal() == wxID_YES)
        callback(true);
    else
        callback(false);

    if (dlg.IsCheckBoxChecked())
        cbc_callback(true);
}

void DialogManager::show_info_dialog(const std::string& text, const std::string& title)
{
    MessageDialog(
        nullptr,
        from_u8(text),
        title.empty() ? _L("Information") : from_u8(title),
        wxICON_INFORMATION | wxOK
    )
        .ShowModal();
}

void DialogManager::show_warning_dialog(const std::string& text, const std::string& title)
{
    MessageDialog(
        nullptr,
        from_u8(text),
        title.empty() ? _L("Warning") : from_u8(title),
        wxICON_WARNING | wxOK
    )
        .ShowModal();
}

void DialogManager::show_error_dialog(const std::string& text, const std::string& title)
{
    MessageDialog(
        nullptr,
        from_u8(text),
        title.empty() ? _L("Error") : from_u8(title),
        wxICON_ERROR | wxOK
    )
        .ShowModal();
}
}