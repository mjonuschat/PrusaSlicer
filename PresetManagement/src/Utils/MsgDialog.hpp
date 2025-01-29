///|/ Copyright (c) Prusa Research 2018 - 2022 Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, David Kocík @kocikdav, Lukáš Hejl @hejllukas, Vojtěch Bubník @bubnikv, Vojtěch Král @vojtechkral
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_MsgDialog_hpp_
#define slic3r_MsgDialog_hpp_

#include <string>
#include <unordered_map>

#include <wx/dialog.h>
#include <wx/font.h>
#include <wx/bitmap.h>
#include <wx/msgdlg.h>
#include <wx/richmsgdlg.h>
#include <wx/textctrl.h>
#include <wx/statline.h>

class wxBoxSizer;
class CheckBox;
class wxStaticBitmap;

namespace Slic3r {

namespace GUI {

// A message / query dialog with a bitmap on the left and any content on the right
// with buttons underneath.
struct MsgDialog : wxDialog
{
	MsgDialog(MsgDialog &&) = delete;
	MsgDialog(const MsgDialog &) = delete;
	MsgDialog &operator=(MsgDialog &&) = delete;
	MsgDialog &operator=(const MsgDialog &) = delete;
	virtual ~MsgDialog() = default;

	void SetButtonLabel(wxWindowID btn_id, const wxString& label, bool set_focus = false);

protected:
	enum {
		CONTENT_WIDTH = 70,//50,
		CONTENT_MAX_HEIGHT = 60,
		BORDER = 30,
		VERT_SPACING = 15,
		HORIZ_SPACING = 5,
	};

	MsgDialog(wxWindow *parent, const wxString &title, const wxString &headline, long style = wxOK, wxBitmap bitmap = wxNullBitmap);
	// returns pointer to created button
	wxButton* add_button(wxWindowID btn_id, bool set_focus = false, const wxString& label = wxString());
	// returns pointer to found button or NULL
	wxButton* get_button(wxWindowID btn_id);
	void apply_style(long style);
	void finalize();

	wxFont boldfont;
	wxBoxSizer *content_sizer;
	wxBoxSizer *btn_sizer;
	wxStaticBitmap *logo;
};

class StaticLine: public wxTextCtrl
{
public:
	StaticLine( wxWindow* parent,
				wxWindowID id = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxLI_HORIZONTAL,
				const wxString& name = wxString::FromAscii(wxTextCtrlNameStr))
	: wxTextCtrl(parent, id, wxEmptyString, pos, size!=wxDefaultSize ? size : (style == wxLI_HORIZONTAL ? wxSize(10, 1) : wxSize(1, 10)), wxSIMPLE_BORDER, wxDefaultValidator, name)
	{
		this->Enable(false);
	}
	~StaticLine() {}
};

}
}

#endif
