#include "MainFrame.hpp"

#include "Slic3r/App/Desktop/LeftBar.hpp"

#include <Slic3r/Biz/Platform/Termination.hpp>
#include <Slic3r/App/AppServices.hpp>
#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/WX/StringConversions.hpp>
#include <Slic3r/App/WX/format.hpp>
#include <Slic3r/App/WX/I18N.hpp>
#include <Slic3r/App/WX/MsgDialog.hpp>
#include <Slic3r/App/WX/WebView/WebViewPanel.hpp>

#include <Slic3r/App/Localization.hpp>
#include "Slic3r/App/Browser/BrowserLogicPrintables.hpp"
#include "Slic3r/App/Browser/BrowserLogicConnectPage.hpp"
#include "Slic3r/App/Browser/BrowserLogicLogInRedirect.hpp"

#include "Slic3r/App/WX/Scalable.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/string.h>

#ifdef WIN32
#include <windows.h>
#include <dbt.h>
#include <shlobj.h>
#endif

namespace Slic3r::App::Desktop {

using namespace WX;

// just temporary function to test color mode / font size testing
#ifdef TOP_BAR
static void add_experimets_page(TopBar* top_bar, MainFrame* main_frame)
#else
static void add_experimets_page(TabsBar* top_bar, MainFrame* main_frame)
#endif
{
    wxPanel* test_panel = new wxPanel(top_bar, wxID_ANY);
    w_config()->UpdateDarkUI(test_panel);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    test_panel->SetSizer(main_sizer);
    main_sizer->SetSizeHints(test_panel);

    wxBoxSizer* test_sizer = new wxBoxSizer(wxHORIZONTAL);
    main_sizer->Add(test_sizer, 0, wxEXPAND);

    wxStaticText* test_txt = new wxStaticText(test_panel, wxID_ANY, from_u8("Change: "));
    test_sizer->Add(test_txt, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    ScalableButton* test_btn = new ScalableButton(test_panel, wxID_ANY, "cog", _L("Color mode"));
    test_btn->SetFont(w_config()->bold_font());
    ScalableButton* test_btn2 = new ScalableButton(test_panel, wxID_ANY, "edit", _L("Apply"));
    test_btn2->SetFont(w_config()->bold_font());

    ScalableButton* lang_selection_btn = new ScalableButton(
        test_panel,
        wxID_ANY,
        "language",
        _L("Select the language"),
        wxDefaultSize,
        wxDefaultPosition,
        wxBU_EXACTFIT | wxNO_BORDER,
        24
    );
    lang_selection_btn->SetFont(w_config()->bold_font());

    test_sizer->Add(test_btn, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    wxBoxSizer* test_sizer2 = new wxBoxSizer(wxHORIZONTAL);
    main_sizer->Add(test_sizer2, 0, wxEXPAND);

    main_sizer->Add(lang_selection_btn, 0, wxALL, 40);

    wxStaticText* test_txt2 = new wxStaticText(test_panel, wxID_ANY, from_u8("Text size: "));
    test_sizer2->Add(test_txt2, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    wxTextCtrl* edit_font = new wxTextCtrl(
        test_panel,
        wxID_ANY,
        wxString::Format(from_u8("%d"), w_config()->normal_font().GetPointSize())
    );
    test_sizer2->Add(edit_font, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    test_sizer2->Add(test_btn2, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    test_btn->Bind(wxEVT_BUTTON, [=](wxCommandEvent& e) {
        main_frame->sys_color_changed();

        test_btn->sys_color_changed();
        test_btn2->sys_color_changed();
        lang_selection_btn->sys_color_changed();

        w_config()->UpdateDarkUI(test_txt);
        w_config()->UpdateDarkUI(test_txt2);
        w_config()->UpdateDarkUI(edit_font);
        w_config()->UpdateDarkUI(test_panel);
        test_panel->Refresh();
    });

    test_btn2->Bind(wxEVT_BUTTON, [=](wxCommandEvent& e) {
        int font_sz;
        edit_font->GetValue().ToInt(&font_sz);

        if (w_config()->normal_font().GetPointSize() != font_sz) {
            wxFont font = w_config()->normal_font();
            font.SetPointSize(font_sz);
            w_config()->update_fonts(font, w_config()->em_unit());
            w_config()->force_fonts_update(main_frame, true);
            main_frame->update_canvas_ui_settings();
        }

        test_panel->Layout();
    });

    lang_selection_btn->Bind(wxEVT_BUTTON, [=](wxCommandEvent& e) { main_frame->select_language(); });

    top_bar->AddPage(test_panel, from_u8("UI - test"));
}

MainFrame::MainFrame(Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor)
    // PrusaSlicer as a window title here is temporary. When being changed - mind that
    // AppInstanceCheck on Windows expects "PrusaSlicer" in the title.
    :
    wxFrame(nullptr, wxID_ANY, L"PrusaSlicer"),
    m_workbench(workbench),
    m_project_interactor(project_interactor),
    m_preset_interactor(project_interactor.preset_interactor())
{
    localization().add_listener<ILanguageChangedListener>(this);
    auto em = w_config()->em_unit();

    const wxSize min_size = FromDIP(wxSize(90 * em, 60 * em));
    this->SetMinSize(min_size);
    this->SetSize(min_size);

    wxFont font = w_config()->normal_font();
    w_config()->update_fonts(font, w_config()->em_unit());

    this->SetFont(w_config()->normal_font());
    w_config()->UpdateDarkUI(this);

#ifdef OLD_CODE
    init_top_bar();
    init_plater();
    init_preset_editors();
    complete_and_bind_top_bar();
    update_preset_editors();

#ifndef __WXOSX__
    this->Bind(wxEVT_DPI_CHANGED, [this](wxDPIChangedEvent& event) {
        event.Skip();
        m_top_bar->Rescale();
        for (auto& [type, panel] : m_preset_editors)
            panel->msw_rescale();

        update_canvas_ui_settings();
    });
#endif
#endif // OLD_CODE

    init_left_bar(project_interactor);
    complete_and_bind_left_bar();

    m_tabs_bar_menus.set_account_menu_callbacks(
        [&project_interactor]() {
            if (!project_interactor.user_account_interactor().is_logged_in()) {
                AppServices::instance().dialog_manager().show_webview_dialog(
                    std::make_unique<Browser::BrowserLogicLogInRedirect>(
                        project_interactor.user_account_interactor()
                    ),
                    &project_interactor
                );
            } else {
                project_interactor.user_account_interactor().do_log_out(true);
            }
        },
        []() {
        } // TODO finish with preferences options
        ,
        [&project_interactor]() {
            return TabsBarMenus::UserAccountInfo{
                project_interactor.user_account_interactor().is_logged_in(),
                project_interactor.user_account_interactor().username(),
                project_interactor.user_account_interactor().avatar()
            };
        }
    );

    project_interactor.user_account_interactor().set_update_menu_callback([this](bool avatar) {
        m_tabs_bar_menus.UpdateAccountMenu();
        m_left_bar->GetLeftBarCtrl()->UpdateAccountButton(avatar);
    });

    project_interactor.user_account_interactor().set_on_logged_in_callback([this]() {
        this->Show(true);
        this->Raise();
        this->SetFocus();
    });

    this->Bind(wxEVT_SYS_COLOUR_CHANGED, [this](wxSysColourChangedEvent& event) {
        event.Skip();
        m_left_bar->OnColorsChanged();
    });

#ifndef __WXOSX__
    this->Bind(wxEVT_DPI_CHANGED, [this](wxDPIChangedEvent& event) {
        event.Skip();
        m_left_bar->Rescale();
        update_canvas_ui_settings();
    });
#endif

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::on_close, this);

    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
#ifdef _WIN32
    // TODO
    // wxGetApp().other_instance_message_handler()->update_windows_properties(this);
#endif // WIN32
        event.Skip();
    });
}

MainFrame::~MainFrame()
{
    localization().remove_listener<ILanguageChangedListener>(this);
}

void MainFrame::on_language_changed()
{
    // Save language at application config.
    // app_config->set("translation_language", localization().active_language());

    m_canvas->set_language(localization().active_language());
    this->Refresh();
}

void MainFrame::on_close(wxCloseEvent& event)
{
    Slic3r::Biz::Platform::close();
    event.Skip();
}

void MainFrame::init_left_bar(Biz::ProjectInteractor& project_interactor)
{
    m_left_bar = LeftBar::Create(this, &m_tabs_bar_menus);

    init_printer_page(project_interactor);
    init_projects_page();
    init_slicing_page();
    init_printables_page(project_interactor);

    //! experiments just for UI testing
    add_experimets_page(m_left_bar, this);

    m_left_bar->message_button()->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        wxMessageBox(from_u8("Message Clicked"), WX::from_u8("TEST"), wxICON_INFORMATION);
    });
    m_left_bar->notifications_button()->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        wxMessageBox(from_u8("Notifications Clicked"), WX::from_u8("TEST"), wxICON_INFORMATION);
    });
}

// !!! temporary function just for testing
static wxPanel* tmp_panel(wxWindow* parent, const wxString& info_text)
{
    wxPanel* test_panel = new wxPanel(parent);
    w_config()->UpdateDarkUI(test_panel);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    test_panel->SetSizer(main_sizer);
    main_sizer->SetSizeHints(test_panel);

    wxBoxSizer* test_sizer = new wxBoxSizer(wxHORIZONTAL);
    main_sizer->Add(test_sizer, 1, wxALIGN_CENTER_HORIZONTAL);

    wxStaticText* text = new wxStaticText(test_panel, wxID_ANY, info_text);
    text->SetFont(w_config()->bold_font());
    test_sizer->Add(text, 1, wxALIGN_CENTER_VERTICAL);
    return test_panel;
}

void MainFrame::init_printer_page(Biz::ProjectInteractor& project_interactor)
{
    std::unique_ptr<App::Browser::BrowserLogicConnectPage>
        logic = std::make_unique<App::Browser::BrowserLogicConnectPage>(project_interactor);
    WebView::WebViewPanel* webview_panel = new WX::WebView::WebViewPanel(
        m_left_bar,
        std::move(logic),
        false
    );
    project_interactor.user_account_interactor()
        .add_listener<Biz::UserAccount::IUserAccountListener>(webview_panel);
    m_left_bar->AddNewPage(webview_panel, from_u8(L("Printers")), "lb_printers");
}

void MainFrame::init_projects_page()
{
    wxPanel* projects_page = tmp_panel(m_left_bar, from_u8("Here will be shown all projects"));
    m_left_bar->AddNewPage(projects_page, from_u8(L("Projects")), "lb_projects");
}

void MainFrame::init_slicing_page()
{
    m_canvas = std::make_unique<Platform::WX::WXRenderCanvas>(m_left_bar);
    m_left_bar->AddNewPage(m_canvas.get(), from_u8(L("Slicing")), "lb_slicing");
}

void MainFrame::init_printables_page(Biz::ProjectInteractor& project_interactor)
{
    std::unique_ptr<App::Browser::BrowserLogicPrintables>
        logic = std::make_unique<App::Browser::BrowserLogicPrintables>(project_interactor);
    WebView::WebViewPanel* webview_panel = new WX::WebView::WebViewPanel(
        m_left_bar,
        std::move(logic),
        false
    );
    project_interactor.user_account_interactor()
        .add_listener<Biz::UserAccount::IUserAccountListener>(webview_panel);
    m_left_bar->AddNewPage(webview_panel, from_u8(L("Printables")), "lb_printables");
}

void MainFrame::complete_and_bind_left_bar()
{
    int slicing_page_id = m_left_bar->FindPage(m_canvas.get());
    m_left_bar->SetSelection(slicing_page_id);

    m_left_bar->Bind(wxEVT_BOOKCTRL_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {});
}

void MainFrame::sys_color_changed()
{
#ifdef WIN32
    w_config()->force_colors_update(!w_config()->dark_mode(), {this});
#endif

    m_left_bar->OnColorsChanged();
}

static int GetSingleChoiceIndex(
    const wxString& message,
    const wxString& caption,
    const wxArrayString& choices,
    int initialSelection
)
{
#ifdef _WIN32
    wxSingleChoiceDialog dialog(nullptr, message, caption, choices);
    WX::w_config()->UpdateDlgDarkUI(&dialog);
    auto children = dialog.GetChildren();
    for (auto child : children)
        child->SetFont(WX::w_config()->normal_font());

    dialog.SetSelection(initialSelection);
    return dialog.ShowModal() == wxID_OK ? dialog.GetSelection() : -1;
#else
    return wxGetSingleChoiceIndex(message, caption, choices, initialSelection);
#endif
}

bool MainFrame::select_language()
{
    wxArrayString names;
    auto language_infos = localization().languages();
    names.Alloc(language_infos.size());

    // Some valid language should be selected since the application start up.
    const std::string active_language = localization().active_language();
    int init_selection                = -1;
    for (size_t i = 0; i < language_infos.size(); ++i) {
        if (language_infos[i].canonical_name == active_language)
            // The dictionary matches the active language and country.
            init_selection = i;
        names.Add(WX::from_u8(language_infos[i].description));
    }

    const long index = GetSingleChoiceIndex(
        _L("Select the language"),
        _L("Language"),
        names,
        init_selection
    );

    // Try to load a new language.
    if (index != -1 && (init_selection == -1 || init_selection != index)) {
        if (localization().set_language(language_infos[index].canonical_name))
            return true;

        // If something was failed during the set new language:

        wxString message = WX::format_wxstr(
            _L("Switching PrusaSlicer to language %1% failed."),
            language_infos[index].canonical_name
        );
#if !defined(_WIN32) && !defined(__APPLE__)
        // likely some linux system
        message += "\n"
            + WX::format_wxstr(
                       _L(
                           "You may need to reconfigure the missing locales, likely by running the %1% and %2% commands.\n"
                       ),
                       "\"locale-gen\"",
                       "\"dpkg-reconfigure locales\""
            );
#endif
        MessageDialog(this, message, _L("PrusaSlicer - Switching language failed"), wxOK | wxICON_ERROR);
    }
    return false;
}

void MainFrame::update_canvas_ui_settings()
{
    m_canvas->set_language(localization().active_language());
    m_canvas->set_font_size(
        float(w_config()->normal_font().GetPointSize()) * this->GetDPIScaleFactor()
    );
    m_canvas->set_font_global_scale(this->GetDPIScaleFactor());
}

#ifdef WIN32
void MainFrame::register_win32_callbacks()
{
    static GUID GUID_DEVINTERFACE_HID =
        {0x4D1E55B2, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30};

    // Register USB HID (Human Interface Devices) notifications to trigger the 3DConnexion enumeration.
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = {0};
    NotificationFilter.dbcc_size                     = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype               = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid                = GUID_DEVINTERFACE_HID;
    m_hDeviceNotify                                  = ::RegisterDeviceNotification(
        this->GetHWND(),
        &NotificationFilter,
        DEVICE_NOTIFY_WINDOW_HANDLE
    );

    // Using Win32 Shell API to register for media insert / removal events.
    LPITEMIDLIST ppidl;
    if (SHGetSpecialFolderLocation(this->GetHWND(), CSIDL_DESKTOP, &ppidl) == NOERROR) {
        SHChangeNotifyEntry shCNE;
        shCNE.pidl       = ppidl;
        shCNE.fRecursive = TRUE;
        // Returns a positive integer registration identifier (ID).
        // Returns zero if out of memory or in response to invalid parameters.
        m_ulSHChangeNotifyRegister = SHChangeNotifyRegister(
            this->GetHWND(), // Hwnd to receive notification
            SHCNE_DISKEVENTS, // Event types of interest (sources)
            SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED,
            // SHCNE_UPDATEITEM,                                                     // Events of interest - use SHCNE_ALLEVENTS for all events
            WM_USER_MEDIACHANGED, // Notification message to be sent upon the event
            1, // Number of entries in the pfsne array
            &shCNE
        ); // Array of SHChangeNotifyEntry structures that
        // contain the notifications. This array should
        // always be set to one when calling SHChnageNotifyRegister
        // or SHChangeNotifyDeregister will not work properly.
        ASSERT(m_ulSHChangeNotifyRegister != 0); // Shell notification failed
    } else {
        // Failed to get desktop location
        ASSERT(false);
    }

    {
        static constexpr int device_count    = 1;
        RAWINPUTDEVICE devices[device_count] = {0};
        // multi-axis mouse (SpaceNavigator, etc.)
        devices[0].usUsagePage = 0x01;
        devices[0].usUsage     = 0x08;
        if (!RegisterRawInputDevices(devices, device_count, sizeof(RAWINPUTDEVICE))) {
            SPDLOG_ERROR("RegisterRawInputDevices failed");
        }
    }
}
#endif // _WIN32

} // namespace Slic3r::App::Desktop
