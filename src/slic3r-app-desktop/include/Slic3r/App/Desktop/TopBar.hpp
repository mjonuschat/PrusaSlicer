#pragma once

#include "TabsBar.hpp"
#include "TopBarCtrl.hpp"

namespace Slic3r::App::Desktop {

class TabsBarMenus;

class TopBar : public TabsBar
{
private:
    TopBar(
        wxWindow* parent,
        wxWindowID winid = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize)
    : TabsBar(parent, winid, pos, size, wxBK_TOP) {}

public:
    static TopBar* Create(
        wxWindow* parent,
        wxWindowID winid = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize)
    {
        TopBar* tb = new TopBar(parent, winid, pos, size);
        tb->CreateBookCtrl();
        return tb;
    }

    static TopBar* Create(
        wxWindow * parent,
        TabsBarMenus* menus)
    {
        TopBar* tb = new TopBar(parent);
        tb->CreateBookCtrl(menus);
        return tb;
    }

    void update_search(const wxString& search) {
        GetTopBarCtrl()->update_search(search);
    }
    
    TabsBarCtrl::Button* new_button() {
        return GetTopBarCtrl()->new_btn;
    }
    
    TabsBarCtrl::Button* save_button() {
        return GetTopBarCtrl()->save_btn;
    }
    
    TabsBarCtrl::Button* hide_sidebars_button() {
        return GetTopBarCtrl()->hide_sidebars_btn;
    }

private:
    void CreateBookCtrl(TabsBarMenus* menus = nullptr) override
    {
        m_bookctrl = new TopBarCtrl(this, wxHORIZONTAL, menus);
        m_controlSizer->Add(m_bookctrl, wxSizerFlags(1).Expand());
    }

    TopBarCtrl* GetTopBarCtrl() const { return static_cast<TopBarCtrl*>(m_bookctrl); }
};

} // namespace Slic3r::App::Desktop