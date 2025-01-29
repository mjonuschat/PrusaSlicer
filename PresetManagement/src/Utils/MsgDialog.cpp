///|/ Copyright (c) Prusa Research 2018 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, Enrico Turri @enricoturri1966, David Kocík @kocikdav, Lukáš Hejl @hejllukas, Vojtěch Král @vojtechkral
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "MsgDialog.hpp"

#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/statbmp.h>
#include <wx/scrolwin.h>
#include <wx/clipbrd.h>
#include <wx/checkbox.h>
#include <wx/html/htmlwin.h>

#include <boost/algorithm/string/replace.hpp>


namespace Slic3r {
namespace GUI {

MsgDialog::MsgDialog(wxWindow *parent, const wxString &title, const wxString &headline, long style, wxBitmap bitmap)
	: wxDialog(parent ? parent : nullptr, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, boldfont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT))
	, content_sizer(new wxBoxSizer(wxVERTICAL))
	, btn_sizer(new wxBoxSizer(wxHORIZONTAL))
{
#ifdef __APPLE__
    this->SetBackgroundColour(wxGetApp().get_window_default_clr());
#endif
	boldfont.SetWeight(wxFONTWEIGHT_BOLD);

    this->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    this->CenterOnParent();

    auto *main_sizer = new wxBoxSizer(wxVERTICAL);
	auto *topsizer = new wxBoxSizer(wxHORIZONTAL);
	auto *rightsizer = new wxBoxSizer(wxVERTICAL);

	auto *headtext = new wxStaticText(this, wxID_ANY, headline);
	headtext->SetFont(boldfont);
    headtext->Wrap(CONTENT_WIDTH*10);
	rightsizer->Add(headtext);
	rightsizer->AddSpacer(VERT_SPACING);

	rightsizer->Add(content_sizer, 1, wxEXPAND);
    btn_sizer->AddStretchSpacer();

	logo = new wxStaticBitmap(this, wxID_ANY, bitmap.IsOk() ? bitmap : wxNullBitmap);

	topsizer->Add(logo, 0, wxALL, BORDER);
	topsizer->Add(rightsizer, 1, wxTOP | wxBOTTOM | wxRIGHT | wxEXPAND, BORDER);

    main_sizer->Add(topsizer, 1, wxEXPAND);
    main_sizer->Add(new StaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, HORIZ_SPACING);
    main_sizer->Add(btn_sizer, 0, wxALL | wxEXPAND, VERT_SPACING);

    apply_style(style);

	SetSizerAndFit(main_sizer);
}

void MsgDialog::SetButtonLabel(wxWindowID btn_id, const wxString& label, bool set_focus/* = false*/) 
{
    if (wxButton* btn = get_button(btn_id)) {
        btn->SetLabel(label);
        if (set_focus)
            btn->SetFocus();
    }
}

wxButton* MsgDialog::add_button(wxWindowID btn_id, bool set_focus /*= false*/, const wxString& label/* = wxString()*/)
{
    wxButton* btn = new wxButton(this, btn_id, label);
    //wxGetApp().SetWindowVariantForButton(btn);
    if (set_focus) {
        btn->SetFocus();
        // For non-MSW platforms SetFocus is not enought to use it as default, when the dialog is closed by ENTER
        // We have to set this button as the (permanently) default one in its dialog
        // See https://twitter.com/ZMelmed/status/1472678454168539146
        btn->SetDefault();
    }
    btn_sizer->Add(btn, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, HORIZ_SPACING);
    btn->Bind(wxEVT_BUTTON, [this, btn_id](wxCommandEvent&) { this->EndModal(btn_id); });
    return btn;
};

wxButton* MsgDialog::get_button(wxWindowID btn_id){
    return static_cast<wxButton*>(FindWindowById(btn_id, this));
}

void MsgDialog::apply_style(long style)
{
    if (style & wxOK)       add_button(wxID_OK, true);
    if (style & wxYES)      add_button(wxID_YES,   !(style & wxNO_DEFAULT));
    if (style & wxNO)       add_button(wxID_NO,     (style & wxNO_DEFAULT));
    if (style & wxCANCEL)   add_button(wxID_CANCEL, (style & wxCANCEL_DEFAULT));

    std::string icon_name = style & wxICON_WARNING        ? "exclamation" :
                            style & wxICON_INFORMATION    ? "info"        :
                            style & wxICON_QUESTION       ? "question"    : "PrusaSlicer";
    //logo->SetBitmap(*get_bmp_bundle(icon_name, 64));
}

void MsgDialog::finalize()
{
    Fit();
    this->CenterOnParent();
}
}
}
