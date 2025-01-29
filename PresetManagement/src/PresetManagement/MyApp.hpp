#ifndef MY_APP_H
#define MY_APP_H

#include "PresetUpdater/PresetUpdaterWrapper.hpp"

#include <wx/wx.h>
#include <memory>

class MyFrame : public wxFrame {
public:
    MyFrame(const wxString& title);
    wxMenuItem* menuItem;
};

class MyApp : public wxApp {
public:
    virtual bool OnInit() override;
private:
    std::unique_ptr<PresetManagement::PresetUpdaterWrapper> m_preset_updater_wrapper; 

    void OnMenuClick(wxCommandEvent& event);
    MyFrame* frame;
};



#endif // MY_APP_H