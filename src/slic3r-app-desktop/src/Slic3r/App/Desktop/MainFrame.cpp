#include "MainFrame.hpp"

#include "Slic3r/Version.hpp"
#include "Slic3r/Directories.hpp"

#include "Slic3r/App/Desktop/LeftBar.hpp"

#include <Slic3r/Biz/Platform/Termination.hpp>
#include <Slic3r/App/AppServices.hpp>
#include "Slic3r/App/AppConfig.hpp"
#include "Slic3r/App/AppConfigInteractor.hpp"
#include "Slic3r/App/IDialogManager.hpp"
#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/WX/StringConversions.hpp>
#include <Slic3r/App/WX/format.hpp>
#include <Slic3r/App/WX/I18N.hpp>
#include <Slic3r/App/WX/MsgDialog.hpp>
#include <Slic3r/App/WX/WebView/WebViewFactory.hpp>

#include <Slic3r/App/Localization.hpp>
#include "Slic3r/App/Navigator.hpp"
#include "Slic3r/App/Browser/BrowserLogicPrintables.hpp"
#include "Slic3r/App/Browser/BrowserLogicConnectPage.hpp"
#include "Slic3r/App/Browser/BrowserLogicLogInRedirect.hpp"

#include "Slic3r/App/WX/Scalable.hpp"
#include "Slic3r/App/WX/WindowMetrics.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"

#include "boost/algorithm/string.hpp"

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/string.h>
#include <wx/display.h>
#include <wx/toplevel.h>

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

    ScalableButton* lang_selection_btn = new ScalableButton(test_panel, wxID_ANY, "language", _L("Select the language"), wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, 24);
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

    test_btn->Bind(
        wxEVT_BUTTON,
        [=](wxCommandEvent& e)
        {
            main_frame->sys_color_changed();

            test_btn->sys_color_changed();
            test_btn2->sys_color_changed();
            lang_selection_btn->sys_color_changed();

            w_config()->UpdateDarkUI(test_txt);
            w_config()->UpdateDarkUI(test_txt2);
            w_config()->UpdateDarkUI(edit_font);
            w_config()->UpdateDarkUI(test_panel);
            test_panel->Refresh();
        }
    );

    test_btn2->Bind(
        wxEVT_BUTTON,
        [=](wxCommandEvent& e)
        {
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
        }
    );

    lang_selection_btn->Bind(wxEVT_BUTTON, [=](wxCommandEvent& e) { main_frame->select_language(); });

#ifdef DEBUG
    top_bar->AddPage(test_panel, from_u8("UI - test"));
#endif
}

// Load the icon either from the exe, or from the ico file.
static wxIcon main_frame_icon()
{
#if _WIN32
    std::wstring path(size_t(MAX_PATH), wchar_t(0));
    int len = int(::GetModuleFileName(nullptr, path.data(), MAX_PATH));
    if (len > 0 && len < MAX_PATH) {
        path.erase(path.begin() + len, path.end());
    }
    return wxIcon(path, wxBITMAP_TYPE_ICO);
#else // _WIN32
    return wxIcon(WX::from_u8(var("PrusaSlicer_128px.png")), wxBITMAP_TYPE_PNG);
#endif // _WIN32
}

MainFrame::MainFrame(
    Domain::Workbench& workbench,
    Biz::ProjectInteractor& project_interactor,
    Navigator& navigator
) :
    wxFrame(nullptr, wxID_ANY, from_u8(::Slic3r::BUILD_ID), wxDefaultPosition,wxDefaultSize,
        wxDEFAULT_FRAME_STYLE, from_u8("mainframe")),
    m_workbench(workbench),
    m_project_interactor(project_interactor),
    m_preset_interactor(project_interactor.preset_interactor()),
    m_navigator(navigator)
{
    // AppInstanceCheck on Windows expects "PrusaSlicer" in the title
    // (in AppInstanceMessageHandlerWin32.cpp). Better check it.
    ASSERT(boost::contains(into_u8(this->GetTitle()), "PrusaSlicer"));

    // Load the icon either from the exe, or from the ico file.
    SetIcon(main_frame_icon());

    AppServices::instance().app_config_interactor().add_listener<IAppConfigChangedListener>(this);

    localization().add_listener<ILanguageChangedListener>(this);
    auto em = w_config()->em_unit();

    const wxSize min_size = FromDIP(wxSize(110 * em, 60 * em));
    this->SetMinSize(min_size);
    this->SetSize(min_size);

    wxFont font = w_config()->normal_font();
    w_config()->update_fonts(font, w_config()->em_unit());

    this->SetFont(w_config()->normal_font());
    w_config()->UpdateDarkUI(this);

    init_left_bar(project_interactor);
    complete_and_bind_left_bar();

    m_tabs_bar_menus.set_account_menu_callbacks(
        [this, &project_interactor]()
        {
            if (!project_interactor.user_account_interactor().is_logged_in()) {
                AppServices::instance().dialog_manager().show_webview_dialog(
                    std::make_unique<Browser::BrowserLogicLogInRedirect>(project_interactor.user_account_interactor()),
                    &project_interactor
                );
                bool was_maximized = IsMaximized();
                this->Show(true);
                this->Restore();
                this->Raise();
                this->SetFocus();
                // Maximize call fixes 2 issues.
                // - On windows the window did de-maximize without reason.
                // - On linux the windows did not come forward at all.
                this->Maximize(was_maximized);
            } else {
                project_interactor.user_account_interactor().do_log_out(true);
            }
        },
        []() {} // TODO finish with preferences options
        ,
        [&project_interactor]()
        {
            return TabsBarMenus::UserAccountInfo{
                project_interactor.user_account_interactor().is_logged_in(),
                project_interactor.user_account_interactor().username(),
                project_interactor.user_account_interactor().avatar()
            };
        }
    );

    project_interactor.user_account_interactor().set_update_menu_callback(
        [this](bool avatar)
        {
            m_tabs_bar_menus.UpdateAccountMenu();
            m_left_bar->GetLeftBarCtrl()->UpdateAccountButton(avatar);
        }
    );

    project_interactor.set_raise_app_fn(
        [this]()
        {
            bool was_maximized = IsMaximized();
            this->Show(true);
            this->Restore();
            this->Raise();
            this->SetFocus();
            // Maximize call fixes 2 issues.
            // - On windows the window did de-maximize without reason.
            // - On linux the windows did not come forward at all.
            this->Maximize(was_maximized);
        }
    );

    this->Bind(
        wxEVT_SYS_COLOUR_CHANGED,
        [this](wxSysColourChangedEvent& event)
        {
            event.Skip();
            m_left_bar->OnColorsChanged();
        }
    );

#ifndef __WXOSX__
    this->Bind(
        wxEVT_DPI_CHANGED,
        [this](wxDPIChangedEvent& event)
        {
            event.Skip();
            m_left_bar->Rescale();
            update_canvas_ui_settings();
        }
    );
#endif

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::on_close, this);

    Bind(
        wxEVT_SIZE,
        [](wxSizeEvent& event)
        {
#ifdef _WIN32
    // TODO
    // wxGetApp().other_instance_message_handler()->update_windows_properties(this);
#endif // WIN32
            event.Skip();
        }
    );

    persist_window_geometry(this, true);
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

void MainFrame::on_app_config_changed()
{
    update_left_bar();
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
    init_preferences_button();

    //! experiments just for UI testing
    add_experimets_page(m_left_bar, this);
}

void MainFrame::init_preferences_button()
{
    m_left_bar->preferences_button()->Bind(
        wxEVT_BUTTON,
        [this](wxCommandEvent& event)
        {
            wxWindow* selected_page        = m_left_bar->GetCurrentPage();
            const bool is_slicing_selected = selected_page
                && selected_page->GetId() == static_cast<wxWindowID>(LeftBarTabs::Slicing);
            if (m_left_bar->preferences_button()->is_selected() && !is_slicing_selected) {
                // just switch to the Slicing page
                switch_left_tab(LeftBarTabs::Slicing, std::string());
                return;
            }

            const bool newly_selected = !m_left_bar->preferences_button()->is_selected();
            if (newly_selected && !is_slicing_selected) {
                switch_left_tab(LeftBarTabs::Slicing, std::string());
            }
            m_navigator.set_opened_preferences(newly_selected);
        }
    );

    m_left_bar->preferences_button()->Bind(
        wxEVT_UPDATE_UI,
        [this](wxUpdateUIEvent& evt)
        {
            m_left_bar->preferences_button()->set_selected(
                m_navigator.is_opened_preferences()
            );
        }
    );
}

// !!! temporary function just for testing
static wxPanel* tmp_panel(wxWindow* parent, int id, const wxString& info_text)
{
    wxPanel* test_panel = new wxPanel(parent, id);
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
    if (!AppServices::instance().app_config().is_prusa_account_enabled()) {
        return;
    }
    assert(!m_printers_page_added);
    std::unique_ptr<App::Browser::BrowserLogicConnectPage> logic = std::make_unique<App::Browser::BrowserLogicConnectPage>(project_interactor);
    WX::WebView::AbstractWebViewPanel* webview_panel = WebView::new_web_view_panel(m_left_bar, static_cast<int>(LeftBarTabs::Printers), std::move(logic), false);
    project_interactor.user_account_interactor().add_listener<Biz::UserAccount::IUserAccountListener>(webview_panel);
    m_left_bar->InsertNewPage(0, webview_panel, WX::_L("Printers"), "lb_printers");
    m_printers_page_added = true;
}

void MainFrame::init_projects_page()
{
    wxPanel* projects_page = tmp_panel(m_left_bar, static_cast<int>(LeftBarTabs::Projects), from_u8("Here will be shown all projects"));
    m_left_bar->AddNewPage(projects_page, WX::_L("Projects"), "lb_projects");
}

void MainFrame::init_slicing_page()
{
    m_canvas = std::make_unique<Platform::WX::WXRenderCanvas>(m_left_bar,  static_cast<int>(LeftBarTabs::Slicing));
    m_left_bar->AddNewPage(m_canvas.get(), WX::_L("Slicing"), "lb_slicing");
}

void MainFrame::init_printables_page(Biz::ProjectInteractor& project_interactor)
{
    if (!AppServices::instance().app_config().is_printables_enabled()) {
        return;
    }
    assert(!m_printables_page_added);
    std::unique_ptr<App::Browser::BrowserLogicPrintables> logic = std::make_unique<App::Browser::BrowserLogicPrintables>(project_interactor);
    WX::WebView::AbstractWebViewPanel* webview_panel = WebView::new_web_view_panel(m_left_bar, static_cast<int>(LeftBarTabs::Printables), std::move(logic), false);
    project_interactor.user_account_interactor().add_listener<Biz::UserAccount::IUserAccountListener>(webview_panel);
    m_left_bar->AddNewPage(webview_panel, WX::_L("Printables"), "lb_printables");
    webview_panel->set_switch_left_tab_fn(std::bind(&MainFrame::switch_left_tab, this, std::placeholders::_1, std::placeholders::_2));
    m_printables_page_added = true;
}

void MainFrame::complete_and_bind_left_bar()
{
    int slicing_page_id = m_left_bar->FindPage(m_canvas.get());
    m_left_bar->SetSelection(slicing_page_id);

    m_left_bar->Bind(wxEVT_BOOKCTRL_PAGE_CHANGED, [](wxBookCtrlEvent& e) {});
}

static size_t get_tab_index_by_id(LeftBar* left_bar, LeftBarTabs page_id)
{
    for (size_t id = 0; id < left_bar->GetPageCount(); ++id) {
        if (left_bar->GetPage(id)->GetId() == static_cast<wxWindowID>(page_id)) {
            return id;
        }
    }
    return size_t(-1);
};

void MainFrame::update_left_bar()
{
    ASSERT(m_left_bar);

    auto remove_page = [this](LeftBarTabs page_id) -> void
    {
        int page_index = get_tab_index_by_id(m_left_bar, page_id);
        ASSERT(page_index != size_t(-1));
        m_left_bar->RemovePage(page_index);
    };

    bool printables_enabled = AppServices::instance().app_config().is_printables_enabled();
    if (m_printables_page_added && !printables_enabled) {
        remove_page(LeftBarTabs::Printables);
        m_printables_page_added = false;
    } else if (!m_printables_page_added && printables_enabled) {
        init_printables_page(m_project_interactor);
    }

    bool prusa_account_enabled = AppServices::instance().app_config().is_prusa_account_enabled();
    if (m_printers_page_added && !prusa_account_enabled) {
        remove_page(LeftBarTabs::Printers);
        m_printers_page_added = false;
    } else if (!m_printers_page_added && prusa_account_enabled) {
        init_printer_page(m_project_interactor);
    }
    m_left_bar->ShowUserAccount(prusa_account_enabled);
    if (!prusa_account_enabled) {
        m_project_interactor.user_account_interactor().do_log_out(true);
    }

    // Always select the Slicing page after updating the left bar
    if (wxWindow* selected_page = m_left_bar->GetCurrentPage();
        selected_page && selected_page->GetId() != static_cast<wxWindowID>(LeftBarTabs::Slicing))
    {
        switch_left_tab(LeftBarTabs::Slicing, std::string());
    }
}

void MainFrame::switch_left_tab(LeftBarTabs id, const std::string& data)
{
    ASSERT(m_left_bar);

    int page_index = get_tab_index_by_id(m_left_bar, id);
    ASSERT(page_index != size_t(-1) && page_index<m_left_bar->GetPageCount());

    if (id == LeftBarTabs::Printables) {
        WebView::AbstractWebViewPanel* webview_panel =
            dynamic_cast<WebView::AbstractWebViewPanel*>(m_left_bar->GetPage(page_index));
        webview_panel->set_next_show_url(data);
    }
    m_left_bar->SetSelection(page_index);
}

void MainFrame::sys_color_changed()
{
#ifdef WIN32
    w_config()->force_colors_update(!w_config()->dark_mode(), {this});
#endif

    m_left_bar->OnColorsChanged();
}

static int GetSingleChoiceIndex(const wxString& message, const wxString& caption, const wxArrayString& choices, int initialSelection)
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

    const long index = GetSingleChoiceIndex(_L("Select the language"), _L("Language"), names, init_selection);

    // Try to load a new language.
    if (index != -1 && (init_selection == -1 || init_selection != index)) {
        if (localization().set_language(language_infos[index].canonical_name))
            return true;

        // If something was failed during the set new language:

        wxString message = WX::
            format_wxstr(_L("Switching PrusaSlicer to language %1% failed."), language_infos[index].canonical_name);
#if !defined(_WIN32) && !defined(__APPLE__)
        // likely some linux system
        message += "\n" + WX::format_wxstr(_L("You may need to reconfigure the missing locales, likely by running the %1% and %2% commands.\n"), "\"locale-gen\"", "\"dpkg-reconfigure locales\"");
#endif
        MessageDialog(this, message, _L("PrusaSlicer - Switching language failed"), wxOK | wxICON_ERROR);
    }
    return false;
}

void MainFrame::update_canvas_ui_settings()
{
    m_canvas->set_language(localization().active_language());
    m_canvas->set_font_size(float(w_config()->normal_font().GetPointSize()) * this->GetDPIScaleFactor());
    m_canvas->set_font_global_scale(this->GetDPIScaleFactor());
}

#ifdef WIN32
void MainFrame::register_win32_callbacks()
{
    static GUID GUID_DEVINTERFACE_HID = {0x4D1E55B2, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30};

    // Register USB HID (Human Interface Devices) notifications to trigger the 3DConnexion enumeration.
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = {0};
    NotificationFilter.dbcc_size                     = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype               = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid                = GUID_DEVINTERFACE_HID;
    m_hDeviceNotify = ::RegisterDeviceNotification(this->GetHWND(), &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

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

void MainFrame::window_pos_save(wxTopLevelWindow* window, const std::string& name)
{
    if (name.empty()) {
        return;
    }

    WindowMetrics metrics = WindowMetrics::from_window(window);
    const auto config_key = fmt::format("{}_window_metrics", name);

    AppConfig& app_config = AppServices::instance().app_config();
    app_config.set(config_key, metrics.serialize());
    // save changed app_config here, before all action related to a close of application is processed
    app_config.save();
}

void MainFrame::window_pos_restore(wxTopLevelWindow* window, const std::string& name, bool default_maximized)
{
    if (name.empty()) { return; }
    const std::string config_key = fmt::format("{}_window_metrics", name);

    AppConfig& app_config = AppServices::instance().app_config();
    std::string metrics_string = app_config.get<std::string>(config_key);

    if (metrics_string.empty()) {
        window->Maximize(default_maximized);
        return;
    }

    auto metrics = WindowMetrics::deserialize(metrics_string);
    if (!metrics) {
        window->Maximize(default_maximized);
        return;
    }

    const wxRect& rect = metrics->get_rect();

    if (app_config.get<bool>("restore_win_position")) {
        // workaround for crash related to the positioning of the window on secondary monitor
        app_config.record_crash("restore_win_pos", "restore_win_position");
        window->SetPosition(rect.GetPosition());

        // workaround for crash related to the positioning of the window on secondary monitor
        app_config.record_crash("restore_win_size", "restore_win_position");
        window->SetSize(rect.GetSize());

        // invalidate "crash_reason" value if application wasn't crashed
        app_config.resolve_crash(std::string(), "restore_win_position");
    }
    else {
        window->CenterOnScreen();
    }

    window->Maximize(metrics->get_maximized());
}

void MainFrame::window_pos_sanitize(wxTopLevelWindow* window)
{
    int display_idx = wxDisplay::GetFromWindow(window);
    wxRect display;
    if (display_idx == wxNOT_FOUND) {
        display = wxDisplay(0u).GetClientArea();
        window->Move(display.GetTopLeft());
    }
    else {
        display = wxDisplay(display_idx).GetClientArea();
    }

    auto metrics = WindowMetrics::from_window(window);
    metrics.sanitize_for_display(display);
    if (window->GetScreenRect() != metrics.get_rect()) {
        window->SetSize(metrics.get_rect());
    }
}

void MainFrame::persist_window_geometry(wxTopLevelWindow* window, bool default_maximized)
{
    const std::string name = into_u8(window->GetName());

    window->Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent& event) {
        window_pos_save(window, name);
        event.Skip();
        });

    window_pos_restore(window, name, default_maximized);

    on_window_geometry(window, [=]() {
        window_pos_sanitize(window);
        });
}

} // namespace Slic3r::App::Desktop
