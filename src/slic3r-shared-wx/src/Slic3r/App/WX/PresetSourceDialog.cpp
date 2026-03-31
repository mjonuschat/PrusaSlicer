#include "Slic3r/App/WX/PresetSourceDialog.hpp"

#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

#include "Slic3r/App/WX/I18N.hpp"
#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/IDialogManager.hpp"
#include "Slic3r/App/Wildcards.hpp"
#include "Slic3r/App/Platform/IFileExplorerHandler.hpp"

#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/wupdlock.h>
#include <wx/html/htmlwin.h>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/statline.h>

namespace Slic3r::App::WX {

PresetSourceDialog::PresetSourceDialog(const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& repository_info)
    : wxDialog(
        wxTheApp->GetTopWindow(),
        wxID_ANY,
        WX::_L("Preset Sources"),
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    ,m_edited_info_vector(repository_info)
{
    const int em = w_config()->em_unit();
    SetFont(GetFont().Scaled(1.1f));
    init();    

    wxStdDialogButtonSizer* buttons = this->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    buttons->GetAffirmativeButton()->SetLabel( WX::_L("Save && Check Updates"));
    this->SetEscapeId(wxID_CANCEL);
    this->Bind(wxEVT_BUTTON, &PresetSourceDialog::onCloseDialog, this, wxID_CANCEL);
    this->Bind(wxEVT_BUTTON, &PresetSourceDialog::onOkDialog, this, wxID_OK);
    m_main_sizer->Add(buttons, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, em);


    buttons->GetAffirmativeButton()->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& event) {
        event.Enable(has_selections());
    });

    SetSizer(m_main_sizer);
    m_main_sizer->SetSizeHints(this);
}

void PresetSourceDialog::init()
{
    const int em = w_config()->em_unit();
    m_main_sizer = new wxBoxSizer(wxVERTICAL);

    auto online_label = new wxStaticText(this, wxID_ANY,  WX::_L("Online sources"));
    online_label->SetFont(online_label->GetFont().Bold().Scaled(1.3f));

    m_main_sizer->Add(online_label, 0, wxTOP | wxLEFT | wxRIGHT | wxBOTTOM, 1.5 * em);

    // Using html to match design of offline_info bellow
    wxHtmlWindow* online_info = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize(80 * em, 6 * em), wxHW_SCROLLBAR_NEVER);
    online_info->SetBorders(0);
    online_info->SetPage(wxString::FromUTF8(fmt::format(
        "<html><body>{}</body></html>",
        WX::_L("Please, select online sources you want to update profiles from:").ToUTF8().data()
    )));
    m_main_sizer->Add(online_info, 0, wxEXPAND | wxLEFT | wxRIGHT, 1.5 * em);

    m_online_sizer = new wxFlexGridSizer(3, 0.5 * em, 1.5 * em);
    m_online_sizer->AddGrowableCol(1);
    m_online_sizer->AddGrowableCol(2);
    m_online_sizer->SetFlexibleDirection(wxBOTH);

    m_main_sizer->Add(m_online_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 1.5 * em);

    m_main_sizer->AddSpacer(0.5 * em);

    wxBoxSizer* local_header_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto offline_label = new wxStaticText(this, wxID_ANY,  WX::_L("Local sources"));
    offline_label->SetFont(offline_label->GetFont().Bold().Scaled(1.3f));
    local_header_sizer->Add(offline_label, 0, wxALIGN_CENTER_VERTICAL);

    local_header_sizer->AddStretchSpacer(1); 

    m_load_btn = new wxButton(this, wxID_ANY,  WX::_L("Load..."));
    m_load_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) { load_offline_repo(); });
    local_header_sizer->Add(m_load_btn, 0, wxALIGN_CENTER_VERTICAL);

    m_main_sizer->Add(local_header_sizer, 0, wxEXPAND | wxTOP | wxLEFT | wxRIGHT | wxBOTTOM, 1.5 * em);

    // append info line with link on printables.com
    {
        wxHtmlWindow* offline_info = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize(80*em, 8 * em), wxHW_SCROLLBAR_NEVER);
        offline_info->SetBorders(0);

        offline_info->Bind(wxEVT_HTML_LINK_CLICKED, [](wxHtmlLinkEvent& event) {
            //wxGetApp().open_browser_with_warning_dialog(event.GetLinkInfo().GetHref());
            AppServices::instance().dialog_manager().open_in_browser(event.GetLinkInfo().GetHref().utf8_string(), 0);
            event.Skip(false);
        });

        wxString message = wxString::FromUTF8(fmt::format(
            fmt::runtime(WX::_L(
                "As an alternative to online sources, profiles can also be updated by manually loading files containing the updates. "
                "This is mostly useful on computers that are not connected to the internet. "
                "Files containing the configuration updates can be downloaded from {0}our website{1}."
            ).ToUTF8().data()),
            "<a href=\"https://prusa.io/prusaslicer-profiles\">",
            "</a>"
        ));

        offline_info->SetPage(message);

        m_main_sizer->Add(offline_info, 0, wxEXPAND | wxLEFT | wxRIGHT, 1.5 * em);
    }

    m_offline_sizer = new wxFlexGridSizer(6, 0.75 * em, 1.5 * em);
    m_offline_sizer->AddGrowableCol(1);
    m_offline_sizer->AddGrowableCol(2);
    m_offline_sizer->AddGrowableCol(3);
    m_offline_sizer->SetFlexibleDirection(wxHORIZONTAL);
     
    m_main_sizer->Add(m_offline_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 1.5 * em);

    fill_grids();

    m_main_sizer->AddSpacer(1.5 * em);

    this->SetMinSize(wxSize(80 * em, 25 * em));
    m_main_sizer->Fit(this);
    this->CenterOnParent();
}

void PresetSourceDialog::fill_grids()
{
    // clear grids
    m_online_sizer->Clear(true);
    m_offline_sizer->Clear(true);
    m_checkboxes.clear();

    bool has_offline = false;
    const int em = w_config()->em_unit();

    auto add = [this, em](wxWindow* win) { 
        m_online_sizer->Add(win, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 1 * em); 
    };

    // header

    for (const wxString& l : std::initializer_list<wxString>{  L"",  WX::_L("Name"),  WX::_L("Description")}) {
        auto text = new wxStaticText(this, wxID_ANY, l);
        text->SetFont(text->GetFont().Bold());
        add(text);
    }

    // data

    // Find all online repos in m_edited_info_vector
    for (Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo& entry : m_edited_info_vector) {
        if (!entry.descriptor.zip_path.empty()) {
            has_offline = true;
            continue; // skip offline repos
        }
        //auto*  chb = new wxCheckBox(this, wxID_ANY, wxString::FromUTF8(entry.descriptor.name));
        auto* chb = new wxCheckBox(this, wxID_ANY, wxEmptyString);
        chb->SetValue(entry.selected);
        chb->Bind(
            wxEVT_CHECKBOX,
            [this, chb, &entry](wxCommandEvent e)
            {
                if (chb->IsChecked()) {
                    select_repository(entry.descriptor.id, entry.descriptor.uuid);
                } else {
                    entry.selected = false;
                }
                update_has_selection();
            }
        );
        add(chb);
        m_checkboxes.emplace(entry.descriptor.uuid, chb);

        add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(entry.descriptor.name) +  L" "));

        add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(entry.descriptor.description) +  L" "));
    }
    

    if (has_offline) {

        auto add_off = [this](wxWindow* win) { m_offline_sizer->Add(win, 0, wxALIGN_CENTER_VERTICAL); };

        // header

        for (const wxString& l : std::initializer_list<wxString>{  L"",  WX::_L("Name"),  WX::_L("Description"),  WX::_L("Source file"),  L"",  L""}) {
            auto text = new wxStaticText(this, wxID_ANY, l);
             text->SetFont(text->GetFont().Bold());
            add_off(text);
        }

        // data1

        for (Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo& entry : m_edited_info_vector) {
            if (entry.descriptor.zip_path.empty()) {
                continue; // skip online repos
            }
            auto* chb = new wxCheckBox(this, wxID_ANY, wxEmptyString);
            std::string id = entry.descriptor.id;
            std::string uuid = entry.descriptor.uuid;
            chb->SetValue(entry.selected);
            chb->Bind(
                wxEVT_CHECKBOX,
                [this, chb, id, uuid](wxCommandEvent e)
                {
                    if (chb->IsChecked()) {
                        select_repository(id, uuid);
                    } else {
                        auto it = std::find_if(m_edited_info_vector.begin(), m_edited_info_vector.end(),
                            [&uuid](const auto& item) { return item.descriptor.uuid == uuid; });
            
                        if (it != m_edited_info_vector.end()) {
                            it->selected = false;
                        }
                    }
                    update_has_selection();
                }
            );
            add_off(chb);
            m_checkboxes.emplace(entry.descriptor.uuid, chb);

            add_off(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(entry.descriptor.name)));

            add_off(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(entry.descriptor.description)));
            {
                auto path_str = new wxStaticText(
                    this, 
                    wxID_ANY, 
                    wxString::FromUTF8(entry.descriptor.zip_path.string()), 
                    wxDefaultPosition, 
                    wxDefaultSize, 
                    wxST_ELLIPSIZE_MIDDLE
                );
                path_str->SetToolTip(wxString::FromUTF8(entry.descriptor.zip_path.string()));

                add_off(path_str);
            }

            {
                auto* btn = new wxButton(this, wxID_ANY,  WX::_L("Open"));
                btn->SetToolTip( WX::_L("Open folder"));
                btn->Bind(wxEVT_BUTTON, [&entry](wxCommandEvent& event) {
                    AppServices::instance().file_explorer_handler().open_folder(entry.descriptor.zip_path.parent_path().make_preferred().string());
                });
                add_off(btn);
            }

            {
                wxButton* btn = new wxButton(this, wxID_ANY,  WX::_L("Remove"));
                btn->Bind(wxEVT_BUTTON, [this, &entry](wxCommandEvent& event) { remove_offline_repo(entry); });
                add_off(btn);
            }
        }
    }

    update_has_selection();
}

void PresetSourceDialog::select_repository(const std::string& id, const std::string& uuid)
{
    // This is only method that should be used to select repo
    for (auto& info : m_edited_info_vector) {
        if (info.descriptor.id == id) {
            info.selected = (info.descriptor.uuid == uuid);
            
            if (auto it = m_checkboxes.find(info.descriptor.uuid); it != m_checkboxes.end() && it->second) {
                it->second->SetValue(info.selected);
            }
        }
    }
}

void PresetSourceDialog::update()
{

    wxWindowUpdateLocker freeze_guard(this);

    fill_grids();

    this->GetSizer()->Layout();

    if (wxDialog* dlg = dynamic_cast<wxDialog*>(this)) {
        this->Layout();
        this->Refresh();
        dlg->Fit();
    }
    else if (wxWindow* top_parent = this->GetParent()) {
        top_parent->Layout();
        top_parent->Refresh();
    }
}

void PresetSourceDialog::update_has_selection()
{
    m_has_selections = std::any_of(m_edited_info_vector.begin(), m_edited_info_vector.end(), 
        [](const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo& info) {
            return info.selected;
        }
    );
}

void PresetSourceDialog::remove_offline_repo(const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo& info)
{
    remove_local_repository_files(info);

    std::string uuid = info.descriptor.uuid;
    m_edited_info_vector.erase(
        std::remove_if(m_edited_info_vector.begin(), m_edited_info_vector.end(),
            [uuid](const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo& item) {
                return item.descriptor.uuid == uuid; 
            }
        ),
        m_edited_info_vector.end()
    );

    m_checkboxes.erase(uuid);


    update_has_selection();

    if (wxDialog* dlg = dynamic_cast<wxDialog*>(this)) {
        // Invalidate min_size for correct next Layout()
        dlg->SetMinSize(wxDefaultSize);
    }

    update();
}

void PresetSourceDialog::load_offline_repo()
{
    auto callback = [this](bool result, const std::vector<boost::filesystem::path>& file_paths)
    {
        if (file_paths.empty()) {
            return;
        }

        // Iterate through the input files
        for (const auto& path : file_paths) {
            std::string err_msg;
            Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo new_info =  Biz::PresetUpdater::prepare_local_repository_files(path, err_msg);

            if (!err_msg.empty()) {
                std::string msg = "Failed to load Source zip file: " + err_msg;
                SPDLOG_ERROR(msg);
                AppServices::instance().dialog_manager().show_error_dialog(msg, "Load Failed");
                continue;
            }
            new_info.selected = true;
            std::string id = new_info.descriptor.id;
            std::string uuid = new_info.descriptor.uuid;
            m_edited_info_vector.emplace_back(std::move(new_info));
            update_has_selection();
            update();
            select_repository(id, uuid);
        }
    };

    AppServices::instance().dialog_manager().show_file_dialog(FileDialogType::Open, Biz::_u8L("Choose one or more ZIP files"), {}, {}, Wildcards::generate_wildcards(Wildcards::TypeFlag::Zip), callback); 
}

void PresetSourceDialog::onCloseDialog(wxEvent &)
{
     this->EndModal(wxID_CANCEL);
}

void PresetSourceDialog::onOkDialog(wxEvent&)
{
    this->EndModal(wxID_OK);
}

Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector PresetSourceDialog::result()
{
    return m_edited_info_vector;
}

} // namespace Slic3r::App::WX