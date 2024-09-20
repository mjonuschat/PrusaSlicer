#include "MainFrame.hpp"
#include "TopBar.hpp"
#include "Preset/AbstractEditor.hpp"
#include "Preset/EditorPrint.hpp"
#include "Preset/EditorFilament.hpp"
#include "Preset/EditorSLAPrint.hpp"
#include "Preset/EditorSLAMaterial.hpp"
#include "Preset/EditorPrinter.hpp"

#include <Slic3r/App/WX/WidgetsConfig.hpp>

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

    wxStaticText* test_txt = new wxStaticText(test_panel, wxID_ANY, "Change: ");
    test_sizer->Add(test_txt, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    ScalableButton* test_btn = new ScalableButton(test_panel, wxID_ANY, "cog", "Color mode");
    test_btn->SetFont(w_config()->bold_font());
    ScalableButton* test_btn2 = new ScalableButton(test_panel, wxID_ANY, "edit", "Apply");
    test_btn2->SetFont(w_config()->bold_font());

    test_sizer->Add(test_btn, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    wxBoxSizer* test_sizer2 = new wxBoxSizer(wxHORIZONTAL);
    main_sizer->Add(test_sizer2, 0, wxEXPAND);

    wxStaticText* test_txt2 = new wxStaticText(test_panel, wxID_ANY, "Text size: ");
    test_sizer2->Add(test_txt2, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    wxTextCtrl* edit_font = new wxTextCtrl(test_panel, wxID_ANY, wxString::Format("%d", w_config()->normal_font().GetPointSize()));
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
        }

        test_panel->Layout();
    });

    top_bar->AddPage(test_panel, ("UI - test"));
}


MainFrame::MainFrame(Domain::Workbench& workbench, Biz::Preset::PresetInteractor& preset_interactor)
    : wxFrame(nullptr, wxID_ANY, ""), m_workbench(workbench), m_preset_interactor(preset_interactor)
{
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
    m_top_bar->AddPage(m_canvas.get(), ("Plater"));
}

void MainFrame::init_preset_editors()
{
    using namespace Preset;

    const auto printer_tech = m_preset_interactor.selected_config_container_context().printer_technology();
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

}
