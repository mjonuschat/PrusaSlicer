#include "Slic3r/App/WX/DialogManager.hpp"

#include "Slic3r/App/WX/WebView/WebViewFactory.hpp"
#include "Slic3r/App/WX/MsgDialog.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/DiffDialog.hpp"
#include "Slic3r/App/WX/UnsavedChangesDialog.hpp"
#include "Slic3r/App/WX/RammingDialog.hpp"
#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/LoadStepDialog.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include <Slic3r/App/WX/I18N.hpp>
#include "Slic3r/Domain/Preset/Types.hpp"
#include "Slic3r/Log.hpp"

#include "Slic3r/App/AppServices.hpp"
#include "Slic3r/App/AppConfig.hpp"

#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/app.h>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

namespace Slic3r::App::WX {

void
DialogManager::show_file_dialog(FileDialogType dialog_type, const std::string& dialog_title, const boost::filesystem::path& override_dir, const std::string& default_file_name, const std::string& wildcards, const FileCallback& callback)
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

    boost::filesystem::path dir;
    boost::system::error_code ec; 
    if (!override_dir.empty() && boost::filesystem::exists(override_dir, ec) && boost::filesystem::is_directory(override_dir, ec)) {
        dir = override_dir;
    } else if (!m_last_dir.empty() && boost::filesystem::exists(m_last_dir, ec) && boost::filesystem::is_directory(m_last_dir, ec)) {
        dir = m_last_dir;
    }
    wxFileDialog dlg(nullptr, from_u8(dialog_title), from_u8(dir.string()), from_u8(default_file_name), from_u8(wildcards), flags);

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

    // Ensure there is an extension when saving file
    if (dialog_type == FileDialogType::Save) {
        auto& path = out_paths.back();
        if (!path.has_extension()) {
            // Selected filter in dialog
            int filter_index = dlg.GetFilterIndex();
            // Split wildcard to tokens
            wxStringTokenizer tokenizer(from_u8(wildcards), L"|");
            wxString ext_token;
            // Find the extension string corresponding to the filter index
            // The extensions are at odd positions (1, 3, 5...)
            for (int i = 0; i <= filter_index * 2 + 1; ++i) {
                ext_token = tokenizer.GetNextToken();
            }
            // Take only first extension
            wxString ext = wxString(ext_token).AfterFirst('*').BeforeFirst(';');
            path.replace_extension(into_u8(ext));
        }
    }

    if (!out_paths.empty()) {
        if (boost::filesystem::is_directory(out_paths.front(), ec)) {
            m_last_dir = out_paths.front();
        } else if (boost::filesystem::exists(out_paths.front().parent_path(), ec)) {
            m_last_dir = out_paths.front().parent_path();
        } else {
            SPDLOG_ERROR("Failed to resolve new default dialog path: {}", ec.message());
        }
    }
    AppServices::instance().app_config().set<std::string>("last_used_directory", m_last_dir.string());
    callback(true, out_paths);
}


void DialogManager::show_webview_dialog(std::unique_ptr<App::Browser::AbstractBrowserLogic>&& logic, Biz::ProjectInteractor* project_interactor)
{
    std::unique_ptr<WebView::AbstractWebViewDialog> dlg = WebView::new_web_view_dialog(std::move(logic));
    project_interactor->user_account_interactor().add_listener<Biz::UserAccount::IUserAccountListener>(dlg.get());
    dlg->ShowModal();
    project_interactor->user_account_interactor().remove_listener<Biz::UserAccount::IUserAccountListener>(dlg.get());
}

void DialogManager::show_yesno_dialog(const std::string& title, const std::string& text, const YesNoCallback& callback)
{
    MessageDialog dlg(nullptr, from_u8(text), from_u8(title), wxYES_NO);
    if (dlg.ShowModal() == wxID_YES)
        callback(true);
    else
        callback(false);
}

void DialogManager::show_rich_yesno_dialog(const std::string& title, const std::string& text, const std::string& check_text, const YesNoCallback& callback, const CheckBoxCheckedCallback& cbc_callback)
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

void DialogManager::show_info_dialog(const std::string& text, const std::string& title, bool is_marked)
{
    // Use CallAfter because most of InfoDialog call we have from the thread.
    wxTheApp->CallAfter([text, title, is_marked] {
        InfoDialog(nullptr, from_u8(title), from_u8(text), is_marked)
            .ShowModal();
        });
}

void DialogManager::show_warning_dialog(const std::string& text, const std::string& title)
{
    MessageDialog(nullptr, from_u8(text), title.empty() ? _L("Warning") : from_u8(title), wxICON_WARNING | wxOK)
        .ShowModal();
}

void DialogManager::show_error_dialog(const std::string& text, const std::string& title)
{
    MessageDialog(nullptr, from_u8(text), title.empty() ? _L("Error") : from_u8(title), wxICON_ERROR | wxOK)
        .ShowModal();
}

void DialogManager::show_diff_dialog(const Slic3r::Biz::Preset::PresetInteractor& preset_interactor, std::optional<Domain::Preset::PresetKind> kind)
{
    DiffDialog(preset_interactor, kind).ShowModal();
}

Biz::Preset::IPresetDialogManager::PresetsSwitchStates DialogManager::show_unsaved_changes_dialog(
    const std::string& dialog_name,
    const Domain::ConfigPack& config_original,
    const Domain::ConfigPack& config_selected,
    Domain::ConfigPack* config_new_selected,
    const Slic3r::Biz::Preset::PresetSelectionNames& preset_names,
    const Slic3r::Biz::Preset::PresetSelectionNames& preset_names_new
)
{
    UnsavedChangesDialog dlg(
        dialog_name,
        config_original,
        config_selected,
        config_new_selected,
        preset_names,
        preset_names_new
    );
    dlg.ShowModal();
    return dlg.exit_states();
}

std::string DialogManager::show_ramming_dialog(const std::string& ramming_parameters)
{
    RammingDialog dlg(ramming_parameters);
    if (dlg.ShowModal() == wxID_OK) {
        return dlg.get_parameters();
    }
    return ramming_parameters;
}


void DialogManager::show_load_step_dialog(const std::string& filename)
{
    LoadStepDialog dlg(filename, 0., 0., true);
    dlg.ShowModal();
}
} // namespace Slic3r::App::WX
