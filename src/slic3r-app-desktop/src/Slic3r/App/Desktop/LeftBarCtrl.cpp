#include "Slic3r/App/Desktop/LeftBarCtrl.hpp"
#include "Slic3r/App/Desktop/TabsBarMenus.hpp"

#include <Slic3r/App/WX/WidgetsConfig.hpp>
#include <Slic3r/App/WX/BitmapGetters.hpp>
#include <Slic3r/App/WX/StringConversions.hpp>

#include "Slic3r/App/WX/I18N.hpp"

#include <wx/sizer.h>

namespace Slic3r::App::Desktop {

using namespace Slic3r::App::WX;

LeftBarCtrl::LeftBarCtrl(wxWindow* parent, int orient, TabsBarMenus* menus)
: TabsBarCtrl(parent, orient, menus)
{
    auto add_btn = [this](Button* btn) -> void {
        m_second_sizer->Add(btn, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, m_btn_margin); };

    // message_btn = new Button(this, { wxEmptyString, "lb_message_smile", m_action_btn_sz });
    // add_btn(message_btn);

    // notifications_btn = new Button(this, { wxEmptyString, "lb_notifications", m_action_btn_sz });
    // add_btn(notifications_btn);

    m_account_btn = new ButtonWithPopup(this, "user", orient, m_login_icon_sz);
    add_btn(m_account_btn);
    
    m_account_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        m_account_btn->set_selected(true);
        m_menus->Popup(this, &m_menus->account, m_account_btn->get_popup_pos());
    });
}

void LeftBarCtrl::OnColorsChanged()
{
    TabsBarCtrl::OnColorsChanged();

    m_account_btn->sys_color_changed();
    // message_btn->sys_color_changed();
    // notifications_btn->sys_color_changed();

    UpdateAccountButton(true);
}

void LeftBarCtrl::UpdateAccountButton(bool avatar/* = false*/)
{
    if (!m_menus)
        return;

    TabsBarMenus::UserAccountInfo  user_account = m_menus->get_user_account_info();
    const wxString user_name = user_account.is_logged ? from_u8(user_account.user_name) : _L("Log in");
    m_account_btn->SetToolTip(user_name);
    if (avatar) {
        if (user_account.is_logged) {
            ScalableBitmap new_logo(this, user_account.avatar_path, wxSize(m_login_icon_sz, m_login_icon_sz));
            if (new_logo.IsOk())
                m_account_btn->SetBitmapBundle(new_logo.bmp());
            else
                m_account_btn->SetBitmapBundle(*get_bmp_bundle("user", m_login_icon_sz));
        }
        else {
            m_account_btn->SetBitmapBundle(*get_bmp_bundle("user", m_login_icon_sz));
        }
    }

    this->Layout();
}

void LeftBarCtrl::UnselectPopupButtons()
{
    m_account_btn->set_selected(false);
}

void LeftBarCtrl::ShowUserAccount(bool show)
{
    m_account_btn->Show(show);
    this->Layout();
}

} // namespace Slic3r::App::Desktop
