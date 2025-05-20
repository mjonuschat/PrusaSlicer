#include "Slic3r/App/Desktop/TopBarCtrl.hpp"
#include "Slic3r/App/Desktop/TabsBarMenus.hpp"

#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/WX/BitmapGetters.hpp>
#include <Slic3r/App/WX/StringConversions.hpp>

#include "Slic3r/App/WX/I18N.hpp"

#include <wx/sizer.h>
#include <wx/dcclient.h>

namespace Slic3r::App::Desktop {

using namespace Slic3r::App::WX;

static int DARK_BG = 29;
static int LIGHT_BG = 221;

TopBarCtrl::TopBarCtrl(wxWindow* parent, int orient, TabsBarMenus* menus)
: TabsBarCtrl(parent, orient, menus)
{
    this->SetBackgroundColour(w_config()->dark_mode() ? wxColour(DARK_BG, DARK_BG, DARK_BG) : wxColour(LIGHT_BG, LIGHT_BG, LIGHT_BG));

    create_search();
    wxBoxSizer* search_sizer = new wxBoxSizer(wxVERTICAL);
    search_sizer->Add(m_search, 1, wxEXPAND);
    m_second_sizer->Add(search_sizer, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, m_btn_margin);
    m_second_sizer->SetMinSize(wxSize(30 * w_config()->em_unit(this), -1));

    auto add_btn = [this](Button* btn, size_t index) -> void {
        m_first_sizer->Insert(index, btn, 0, wxALIGN_CENTER_VERTICAL | wxALL, 0.5*m_btn_margin); };

    size_t btn_index = 0;
    new_btn = new Button(this, { wxEmptyString, "tb_new", m_btn_icon_sz });
    new_btn->SetToolTip(_L("New Project"));
    add_btn(new_btn, btn_index++);

    load_btn = new Button(this, { wxEmptyString, "tb_load", m_btn_icon_sz });
    load_btn->SetToolTip(_L("Load Project"));
    add_btn(load_btn, btn_index++);

    save_btn = new Button(this, { wxEmptyString, "tb_save", m_btn_icon_sz });
    save_btn->SetToolTip(_L("Save Project"));
    add_btn(save_btn, btn_index++);

    hide_sidebars_btn = new Button(this, { wxEmptyString, "tb_show_ui", m_btn_icon_sz });
    add_btn(hide_sidebars_btn, btn_index++);

    m_first_sizer->InsertSpacer(btn_index, 20);

    this->Bind(wxEVT_PAINT, [this](wxPaintEvent& e) {
        e.Skip();

        // render Button as a Tab => Fill "empty space" under selected button

        wxPaintDC dc(this);
        dc.SetPen(w_config()->get_color_selected_btn_bg());
        dc.SetBrush(w_config()->get_color_selected_btn_bg());

        wxRect rc = get_selected_tab_rect();
        rc.height += w_config()->em_unit(this);
        dc.DrawRectangle(rc);
    });
}

void TopBarCtrl::OnColorsChanged()
{
    TabsBarCtrl::OnColorsChanged();
    this->SetBackgroundColour(w_config()->dark_mode() ? wxColour(DARK_BG, DARK_BG, DARK_BG) : wxColour(LIGHT_BG, LIGHT_BG, LIGHT_BG));

    m_search->SysColorsChanged();

    new_btn->sys_color_changed();
    load_btn->sys_color_changed();
    save_btn->sys_color_changed();
    hide_sidebars_btn->sys_color_changed();
}

void TopBarCtrl::create_search()
{
    // Linux specific: If wxDefaultSize is used in constructor and than set just maxSize, 
    // than this max size will be used as a default control size and can't be resized.
    // So, set initial size for some minimum value
    m_search = new WX::Widgets::TextInput(this, /*wxGetApp().searcher().default_string*/_L("Input"), from_u8(""), from_u8("search"), wxDefaultPosition, wxSize(2 * w_config()->em_unit(this), -1), wxTE_PROCESS_ENTER);
    m_search->SetMaxSize(wxSize(30*w_config()->em_unit(this), -1));
    w_config()->UpdateDarkUI(m_search);
/*
    m_search->Bind(wxEVT_TEXT, [](wxEvent& e)
    {
        wxGetApp().searcher().edit_search_input();
        wxGetApp().update_search_lines();
    });

    m_search->Bind(wxEVT_MOVE, [](wxMoveEvent& event)
    { 
        event.Skip();
        wxGetApp().searcher().update_dialog_position();
    });

    m_search->SetOnDropDownIcon([this]()
    {
        wxGetApp().searcher().set_search_input(m_search);
        wxGetApp().show_search_dialog(); 
    });

    m_search->Bind(wxEVT_KILL_FOCUS, [](wxFocusEvent& e)
    {
        e.Skip();
        wxGetApp().searcher().check_and_hide_dialog();
    });

    wxTextCtrl* ctrl = m_search->GetTextCtrl();
    ctrl->SetToolTip(format_wxstr(_L("Search in settings [%1%]"), "Ctrl+F"));

    ctrl->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e)
    {
        wxGetApp().searcher().set_search_input(m_search); 
        if (e.GetKeyCode() == WXK_TAB)
            m_search->Navigate(e.ShiftDown() ? wxNavigationKeyEvent::IsBackward : wxNavigationKeyEvent::IsForward);
        else
            wxGetApp().searcher().process_key_down_from_input(e);
        e.Skip();
    });

    ctrl->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& event)
    {
        wxGetApp().searcher().set_search_input(m_search);
        wxGetApp().show_search_dialog();
        event.Skip();
    });

    ctrl->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& event)
    {
        if (m_search->GetValue() == wxGetApp().searcher().default_string)
            m_search->SetValue("");
        event.Skip();
    });
*/
}

void TopBarCtrl::update_search(const wxString& search)
{
    if (search != m_search->GetValue())
        m_search->SetValue(search);
}

} // namespace Slic3r::App::Desktop