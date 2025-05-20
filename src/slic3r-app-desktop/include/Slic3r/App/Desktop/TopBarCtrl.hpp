#pragma once

#include "TabsBarCtrl.hpp"

#include "Slic3r/App/WX/Widgets/TextInput.hpp"

namespace Slic3r::App::Desktop {

class TopBarCtrl : public TabsBarCtrl {

public:
    TopBarCtrl(wxWindow* parent,
        int orient,
        TabsBarMenus* menus = nullptr);
    ~TopBarCtrl() = default;

    void OnColorsChanged() override;
    void UnselectPopupButtons() override {};

    void update_search(const wxString& search);
//    wxWindow* get_search_ctrl() { return m_search->GetTextCtrl(); */}

private:
    void create_search();

public:
    Button* new_btn             { nullptr };
    Button* load_btn             { nullptr };
    Button* save_btn            { nullptr };
    Button* hide_sidebars_btn   { nullptr };

private:
    WX::Widgets::TextInput* m_search        { nullptr };
    int                     m_btn_icon_sz   { 14 };

};

} // namespace Slic3r::App::Desktop