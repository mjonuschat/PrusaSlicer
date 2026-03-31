#pragma once

#include "Slic3r/Biz/PresetUpdater/PresetUpdaterRepositoryDescriptor.hpp"

#include <wx/dialog.h>
#include <set>
#include <map>

class wxWindow;
class wxEvent;
class wxSizer;
class wxFlexGridSizer;
class wxCheckBox;

namespace Slic3r::App::WX {

class PresetSourceDialog : public wxDialog
{
public:
    PresetSourceDialog(
        const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector& repository_info
    );
    ~PresetSourceDialog() = default;

    Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector result();

private:
    Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfoVector m_edited_info_vector;
    wxSizer* m_main_sizer{nullptr};
    wxFlexGridSizer* m_online_sizer{nullptr};
    wxFlexGridSizer* m_offline_sizer{nullptr};
    wxButton* m_load_btn{nullptr};
    bool m_has_selections{false};
    std::map<std::string, wxCheckBox*> m_checkboxes;

    void init();
    void fill_grids();
    void select_repository(const std::string& id, const std::string& uuid);
    void remove_offline_repo(const Biz::PresetUpdater::SharedPresetUpdaterRepositoryInfo& info);
    void load_offline_repo();
    void update_has_selection();

    void onCloseDialog(wxEvent&);
    void onOkDialog(wxEvent&);

    void update();

    bool has_selections() const
    {
        return m_has_selections;
    }

};

} // namespace Slic3r::App::WX
