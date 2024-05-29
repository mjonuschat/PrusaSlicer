#include "MainFrame.hpp"
#include "TopBar.hpp"

#include <Slic3r/App/WX/WidgetsConfig.hpp>

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/string.h>

namespace Slic3r::App::Desktop {

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "")
{
    using namespace WX;

    auto em = w_config()->em_unit();

    this->SetMinSize(FromDIP(wxSize(80 * em, 40 * em)));
    this->SetMaxSize(FromDIP(wxSize(120 * em, 80 * em)));


    wxFont font = w_config()->normal_font();
    font.SetPointSize(15);
    w_config()->update_fonts(font, w_config()->em_unit());

    this->SetFont(w_config()->normal_font());
    w_config()->UpdateDarkUI(this);

    m_top_bar_menus.set_workspaces_menu_callbacks(
        [this]()                                -> int { return m_mode; },
        [this](int mode)                        -> void {
            m_mode = mode;
            m_top_bar->UpdateMode();
        },
        [](/*ConfigOptionMode*/int mode)-> std::string { return w_config()->get_mode_btn_color(mode); });

    m_top_bar_menus.ApplyWorkspacesMenu();

    m_top_bar = new TopBar(this, &m_top_bar_menus);
    m_canvas = std::make_unique<Platform::WX::WXRenderCanvas>(m_top_bar);

    m_top_bar->AddPage(m_canvas.get(), ("Plater"));


    //! experiments

    wxPanel* test_panel = new wxPanel(m_top_bar, wxID_ANY);
    w_config()->UpdateDarkUI(test_panel);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    test_panel->SetSizer(main_sizer);
    main_sizer->SetSizeHints(test_panel);

    wxBoxSizer* test_sizer = new wxBoxSizer(wxHORIZONTAL);
    main_sizer->Add(test_sizer, 1, wxEXPAND);

    wxStaticText* test_txt = new wxStaticText(test_panel, wxID_ANY, "Change: ");
    test_sizer->Add(test_txt, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    ScalableButton* test_btn = new ScalableButton(test_panel, wxID_ANY, "cog", "Color mode");
    test_btn->SetFont(w_config()->bold_font());
    ScalableButton* test_btn2 = new ScalableButton(test_panel, wxID_ANY, "edit", "Apply");
    test_btn2->SetFont(w_config()->bold_font());

    test_sizer->Add(test_btn, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);


    wxBoxSizer* test_sizer2 = new wxBoxSizer(wxHORIZONTAL);
    main_sizer->Add(test_sizer2, 1, wxEXPAND);

    wxStaticText* test_txt2 = new wxStaticText(test_panel, wxID_ANY, "Text size: ");
    test_sizer2->Add(test_txt2, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    wxTextCtrl* edit_font = new wxTextCtrl(test_panel, wxID_ANY, wxString::Format("%d", w_config()->normal_font().GetPointSize()));
    test_sizer2->Add(edit_font, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    test_sizer2->Add(test_btn2, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 20);

    test_btn->Bind(wxEVT_BUTTON, [=](wxCommandEvent& e) {
        w_config()->force_colors_update(!w_config()->dark_mode(), {this});
        m_top_bar->OnColorsChanged();
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
        }

        this->SetFont(w_config()->normal_font());
        test_txt2->SetFont(w_config()->normal_font());
        edit_font->SetFont(w_config()->normal_font());
        test_btn2->SetFont(w_config()->bold_font());

        test_panel->Layout();
    });

    //!

    m_top_bar->AddPage(test_panel, ("UI - test"));

    m_top_bar->SetSelection(0);
    m_top_bar->UpdateMode();

    this->Bind(wxEVT_SYS_COLOUR_CHANGED, [this](wxSysColourChangedEvent& event)
    {
        event.Skip();
        m_top_bar->OnColorsChanged();
    });
}

}
