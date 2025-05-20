#include "MainFrame.hpp"
#include "Preset/AbstractEditor.hpp"
#include "Preset/EditorPrint.hpp"
#include "Preset/EditorFilament.hpp"
#include "Preset/EditorSLAPrint.hpp"
#include "Preset/EditorSLAMaterial.hpp"
#include "Preset/EditorPrinter.hpp"

#include "Slic3r/App/Desktop/LeftBar.hpp"
#include "Slic3r/App/Desktop/TopBar.hpp"

#include <Slic3r/Biz/Platform/Termination.hpp>
#include <Slic3r/App/IDialogManager.hpp>
#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/WX/StringConversions.hpp>
#include <Slic3r/App/WX/format.hpp>
#include <Slic3r/App/WX/I18N.hpp>
#include <Slic3r/App/WX/MsgDialog.hpp>

#include <Slic3r/App/Localization.hpp>

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/string.h>

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

    ScalableButton* lang_selection_btn = new ScalableButton(test_panel, wxID_ANY, "language", _L("Select the language"),
                                                            wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, 24);
    lang_selection_btn->SetFont(w_config()->bold_font());

    test_sizer->Add(test_btn, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    wxBoxSizer* test_sizer2 = new wxBoxSizer(wxHORIZONTAL);
    main_sizer->Add(test_sizer2, 0, wxEXPAND);

    main_sizer->Add(lang_selection_btn, 0, wxALL, 40);

    wxStaticText* test_txt2 = new wxStaticText(test_panel, wxID_ANY, from_u8("Text size: "));
    test_sizer2->Add(test_txt2, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    wxTextCtrl* edit_font = new wxTextCtrl(test_panel, wxID_ANY, wxString::Format(from_u8("%d"), w_config()->normal_font().GetPointSize()));
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

    lang_selection_btn->Bind(wxEVT_BUTTON, [=](wxCommandEvent& e) {
        main_frame->select_language();
    });

    top_bar->AddPage(test_panel, from_u8("UI - test"));
}

MainFrame::MainFrame(
    Domain::Workbench& workbench,
    Biz::ProjectInteractor& project_interactor
)
    // PrusaSlicer as a window title here is temporary. When being changed - mind that
    // AppInstanceCheck on Windows expects "PrusaSlicer" in the title.
    : wxFrame(nullptr, wxID_ANY, L"PrusaSlicer")
    , m_workbench(workbench)
    , m_project_interactor(project_interactor)
    , m_preset_interactor(project_interactor.preset_interactor())
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
    this->Bind(wxEVT_DPI_CHANGED, [this](wxDPIChangedEvent& event)
    {
        event.Skip();
        m_top_bar->Rescale();
        for (auto& [type, panel] : m_preset_editors)
            panel->msw_rescale();

        update_canvas_ui_settings();
    });
#endif
#endif // OLD_CODE

    init_left_bar();
    complete_and_bind_left_bar();

    this->Bind(wxEVT_SYS_COLOUR_CHANGED, [this](wxSysColourChangedEvent& event)
    {
        event.Skip();
        m_top_bar->OnColorsChanged();
        m_left_bar->OnColorsChanged();
    });

#ifndef __WXOSX__
    this->Bind(wxEVT_DPI_CHANGED, [this](wxDPIChangedEvent& event)
    {
        event.Skip();
        m_left_bar->Rescale();
        m_top_bar->Rescale();
        update_canvas_ui_settings();
    });
#endif

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::on_close, this);

    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
#ifdef _WIN32
        // TODO
        //wxGetApp().other_instance_message_handler()->update_windows_properties(this);
#endif //WIN32
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
    //app_config->set("translation_language", localization().active_language());

    m_canvas->set_language(localization().active_language());
    this->Refresh();
}

void MainFrame::on_close(wxCloseEvent& event)
{
    Slic3r::Biz::Platform::close();
    event.Skip();
}

#ifdef OLD_CODE
void MainFrame::init_top_bar()
{
    m_top_bar_menus.set_workspaces_menu_callbacks(
        [this]()                                -> int { return m_mode; },
        [this](int mode)                        -> void {
            m_mode = mode;
            m_top_bar->UpdateMode();
        },
        [](/*ConfigOptionMode*/int mode)-> std::string { return w_config()->get_mode_btn_color(mode); });

    m_top_bar_menus.ApplyWorkspacesMenu();

    m_top_bar = new TopBar(this, &m_top_bar_menus);
}

void MainFrame::update_preset_editors()
{
    for (auto& [type, panel] : m_preset_editors)
        panel->update_selected_ccc();
}

void MainFrame::complete_and_bind_top_bar()
{
    m_top_bar->SetSelection(0);
    m_top_bar->UpdateMode();

    m_top_bar->Bind(wxEVT_BOOKCTRL_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
        if (int old_selection = e.GetOldSelection();
            old_selection != wxNOT_FOUND && old_selection < static_cast<int>(m_top_bar->GetPageCount())) {
            Preset::AbstractEditor* old_editor = dynamic_cast<Preset::AbstractEditor*>(m_top_bar->GetPage(old_selection));
            if (old_editor)
                old_editor->validate_custom_gcodes();
        }

        wxWindow* panel = m_top_bar->GetCurrentPage();
        Preset::AbstractEditor* editor = dynamic_cast<Preset::AbstractEditor*>(panel);

        const auto& ccc = m_preset_interactor.selected_config_container_context();

        // There shouldn't be a case, when we try to select a editor, which doesn't support a printer technology
        if (!panel || (editor && !editor->supports_printer_technology(ccc.printer_technology())))
            return;

        // temporary fix - WebViewPanel is not inheriting from Tab -> would jump to select Plater
        if (panel && !editor)
            return;

        if (editor && m_preset_editors.find(editor->type()) != m_preset_editors.end() && m_preset_editors[editor->type()] == editor) {
            // On GTK, the wxEVT_NOTEBOOK_PAGE_CHANGED event is triggered
            // before the MainFrame is fully set up.
            editor->activate();
//!            m_last_selected_tab = m_top_bar->GetSelection();
        }
//!        else
//!            select_tab(size_t(0)); // select Plater
    });
}

void MainFrame::init_preset_editors()
{
    using namespace Preset;

    const auto& ccc = m_preset_interactor.selected_config_container_context();
    const auto printer_tech = ccc.printer_technology();
    if (printer_tech == ptFFF) {
        add_preset_editor(new EditorPrint(m_top_bar, m_preset_interactor), "cog");
        add_preset_editor(new EditorFilament(m_top_bar, m_preset_interactor), "spool");
    }
    else {
        add_preset_editor(new EditorSLAPrint(m_top_bar, m_preset_interactor), "cog");
        add_preset_editor(new EditorSLAMaterial(m_top_bar, m_preset_interactor), "resin");
    }
    add_preset_editor(new EditorPrinter(m_top_bar, m_preset_interactor), printer_tech == ptFFF ? "printer" : "sla_printer");
}

void MainFrame::add_preset_editor(Preset::AbstractEditor* panel, const std::string& bmp_name /*= ""*/)
{
    panel->init(&m_preset_interactor);
    m_preset_editors[panel->type()] = panel;

    m_top_bar->AddNewPage(panel, panel->title(), bmp_name);
}

void MainFrame::init_plater()
{
    m_canvas = std::make_unique<Platform::WX::WXRenderCanvas>(m_top_bar);
    m_top_bar->AddPage(m_canvas.get(), from_u8("Plater"));
}
#endif // OLD_CODE

void MainFrame::init_left_bar()
{
    m_left_bar = LeftBar::Create(this, &m_tabs_bar_menus);

    init_printer_page();
    init_projects_page();
    init_slicing_page();
    init_printables_page();

    m_left_bar->message_button()->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        wxMessageBox(from_u8("Message Clicked"), WX::from_u8("TEST"), wxICON_INFORMATION);
    });
    m_left_bar->notifications_button()->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        wxMessageBox(from_u8("Notifications Clicked"), WX::from_u8("TEST"), wxICON_INFORMATION);});
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

void MainFrame::init_printer_page()
{
    wxPanel* printers_page = tmp_panel(m_left_bar, from_u8("Here will be shown all printers"));
    m_left_bar->AddNewPage(printers_page, from_u8(L("Printers")), "lb_printers");
}

void MainFrame::init_projects_page()
{
    wxPanel* projects_page = tmp_panel(m_left_bar, from_u8("Here will be shown all projects"));
    m_left_bar->AddNewPage(projects_page, from_u8(L("Projects")), "lb_projects");
}

void MainFrame::init_slicing_page()
{
    m_slicing_panel = new wxPanel(m_left_bar);
    w_config()->UpdateDarkUI(m_slicing_panel);

    init_top_bar();

    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_top_bar, 1, wxEXPAND);
    m_slicing_panel->SetSizer(main_sizer);

    m_left_bar->AddNewPage(m_slicing_panel, from_u8(L("Slicing")), "lb_slicing");

    complete_and_bind_top_bar();
}

void MainFrame::init_printables_page()
{
    wxPanel* printables_page = tmp_panel(m_left_bar, from_u8("Here will be shown Printables web page"));
    m_left_bar->AddNewPage(printables_page, from_u8(L("Printables")), "lb_printables");
}

void MainFrame::complete_and_bind_left_bar()
{
    int slicing_page_id = m_left_bar->FindPage(m_slicing_panel);
    m_left_bar->SetSelection(slicing_page_id);

    m_left_bar->Bind(wxEVT_BOOKCTRL_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
        });
}

void MainFrame::init_top_bar()
{
    m_top_bar = TopBar::Create(m_slicing_panel);

    m_canvas = std::make_unique<Platform::WX::WXRenderCanvas>(m_top_bar);
    m_top_bar->AddPage(m_canvas.get(), from_u8(L("New Project")));

    //! experiments just for UI testing
    add_experimets_page(m_top_bar, this);

    m_top_bar->new_button()->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        wxMessageBox(from_u8("\"New\" button is Clicked"), WX::from_u8("TEST"), wxICON_INFORMATION);
    });

    m_top_bar->load_button()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        load_project();
    });

    m_top_bar->save_button()->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        wxMessageBox(from_u8("\"Save\" button is Clicked"), WX::from_u8("TEST"), wxICON_INFORMATION);
    });

    auto update_hide_sidebars_button = [this](bool hide) -> void {
        m_top_bar->hide_sidebars_button()->set_selected(hide);
        m_top_bar->hide_sidebars_button()->SetToolTip(hide ? _L("Show sidebars") : _L("Hide sidebars")); 
    };

    m_top_bar->hide_sidebars_button()->Bind(wxEVT_BUTTON, [update_hide_sidebars_button, this](wxCommandEvent&) {
        bool hide = !m_top_bar->hide_sidebars_button()->is_selected();
        update_hide_sidebars_button(hide);

        // Propagate sidebars visibility into active RenderModule
        // ???Is it a good idea to change render module from MainFrame
        m_canvas->get_render_module()->set_sidebars_visible(hide);

        // ysTODO: save hide value into app_config
    });

    bool hide_sidebars{ false };// ysTODO: get value from app_config
    update_hide_sidebars_button(hide_sidebars);
}

void MainFrame::complete_and_bind_top_bar()
{
    int canvas_page_id = m_top_bar->FindPage(m_canvas.get());
    m_top_bar->SetSelection(canvas_page_id);
    m_top_bar->Bind(wxEVT_BOOKCTRL_PAGE_CHANGED, [this](wxBookCtrlEvent& e) {
        });
}

void MainFrame::sys_color_changed()
{
#ifdef WIN32
    w_config()->force_colors_update(!w_config()->dark_mode(), { this });
#endif

    m_top_bar->OnColorsChanged();
    m_left_bar->OnColorsChanged();
}

static int GetSingleChoiceIndex(const wxString& message,
                                const wxString& caption,
                                const wxArrayString& choices,
                                int initialSelection)
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
    int init_selection = -1;
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

        wxString message = WX::format_wxstr(_L("Switching PrusaSlicer to language %1% failed."), language_infos[index].canonical_name);
#if !defined(_WIN32) && !defined(__APPLE__)
        // likely some linux system
        message += "\n" + WX::format_wxstr(_L("You may need to reconfigure the missing locales, likely by running the %1% and %2% commands.\n"),
                                              "\"locale-gen\"", "\"dpkg-reconfigure locales\"");
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

void MainFrame::load_project()
{
    auto& dlg_manager = DialogManagerProvider::instance().get();


    IDialogManager::FileCallback callback = [this](bool success, const boost::filesystem::path& file_path)
    {
        if (success)
            m_project_interactor.load_project(file_path.string());
    };

    dlg_manager.show_file_dialog(FileDialogType::Open, "Open Project", "", "", "*.3mf", callback);


}


}
