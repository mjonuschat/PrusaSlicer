#include "MainFrame.hpp"
#include "TopBar.hpp"
#include "Preset/AbstractEditor.hpp"
#include "Preset/EditorPrint.hpp"
#include "Preset/EditorFilament.hpp"
#include "Preset/EditorSLAPrint.hpp"
#include "Preset/EditorSLAMaterial.hpp"
#include "Preset/EditorPrinter.hpp"

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
static void add_experimets_page(TopBar* top_bar, MainFrame* main_frame)
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
            main_frame->get_render_canvas().set_font_size(1.7777f * float(font_sz));
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
    Biz::Preset::PresetInteractor& preset_interactor
)
    : wxFrame(nullptr, wxID_ANY, {}), m_workbench(workbench), m_preset_interactor(preset_interactor)
{
    localization().add_language_changed_listener(this);
    auto em = w_config()->em_unit();

    this->SetMinSize(FromDIP(wxSize(80 * em, 40 * em)));

    wxFont font = w_config()->normal_font();
    w_config()->update_fonts(font, w_config()->em_unit());

    this->SetFont(w_config()->normal_font());
    w_config()->UpdateDarkUI(this);

    init_top_bar();

    init_plater();

    init_preset_editors();

    //! experiments just for UI testing
    add_experimets_page(m_top_bar, this);

    complete_and_bind_top_bar();

    update_preset_editors();

    this->Bind(wxEVT_SYS_COLOUR_CHANGED, [this](wxSysColourChangedEvent& event)
    {
        event.Skip();
        m_top_bar->OnColorsChanged();
    });

#ifndef __WXOSX__
    this->Bind(wxEVT_DPI_CHANGED, [this](wxDPIChangedEvent& event)
    {
        event.Skip();
        m_top_bar->Rescale();
        for (auto& [type, panel] : m_preset_editors)
            panel->msw_rescale();
    });
#endif
}

MainFrame::~MainFrame()
{
    localization().remove_language_changed_listener(this);
}

void MainFrame::on_language_changed()
{
    // Save language at application config.
    //app_config->set("translation_language", localization().active_language());

    m_canvas->set_language(localization().active_language());
    this->Refresh();
}

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

void MainFrame::init_plater()
{
    m_canvas = std::make_unique<Platform::WX::WXRenderCanvas>(m_top_bar);
    m_top_bar->AddPage(m_canvas.get(), from_u8("Plater"));
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

void MainFrame::sys_color_changed()
{
#ifdef WIN32
    w_config()->force_colors_update(!w_config()->dark_mode(), { this });
#endif
    m_top_bar->OnColorsChanged();

    for (auto& [type, panel] : m_preset_editors)
        panel->sys_color_changed();

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

}
