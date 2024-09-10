#pragma once

#include <vector>

#include <wx/dialog.h>
#include <wx/dataview.h>
#include <wx/gdicmn.h>

#include "libslic3r/Preset.hpp"
#include "libslic3r/PrintConfig.hpp"

class wxListBox;
class wxTextCtrl;
class wxStaticText;

namespace Slic3r::App::WX {
    class ScalableButton;
}

namespace Slic3r::Biz::Preset {
    struct  PresetInteractorConfigContainerContext;
}

namespace Slic3r::App::Desktop::Preset {

class ParamsViewCtrl;

//------------------------------------------
//          EditGCodeDialog
//------------------------------------------

class EditGCodeDialog : public wxDialog
{
public:
    EditGCodeDialog(wxWindow*parent, const std::string&key, const std::string&value, 
                    Biz::Preset::PresetInteractorConfigContainerContext* ccc);
    ~EditGCodeDialog();

    std::string get_edited_gcode() const;

private:
    void    init_params_list(const std::string& custom_gcode_name);
    void    add_selected_value_to_gcode();
    void    bind_list_and_button();

    wxDataViewItem add_presets_placeholders();

    void    selection_changed(wxDataViewEvent& evt);
    void    msw_rescale();

private:
    Biz::Preset::PresetInteractorConfigContainerContext* m_ccc  {nullptr};

    ParamsViewCtrl*                 m_params_list               {nullptr};
    WX::ScalableButton*             m_add_btn                   {nullptr};
    wxTextCtrl*                     m_gcode_editor              {nullptr};
    wxStaticText*                   m_param_label               {nullptr};
    wxStaticText*                   m_param_description         {nullptr};

    ReadOnlySlicingStatesConfigDef  m_cgp_ro_slicing_states_config_def;
    ReadWriteSlicingStatesConfigDef m_cgp_rw_slicing_states_config_def;
    OtherSlicingStatesConfigDef     m_cgp_other_slicing_states_config_def;
    PrintStatisticsConfigDef        m_cgp_print_statistics_config_def;
    ObjectsInfoConfigDef            m_cgp_objects_info_config_def;
    DimensionsConfigDef             m_cgp_dimensions_config_def;
    TimestampsConfigDef             m_cgp_timestamps_config_def;
    OtherPresetsConfigDef           m_cgp_other_presets_config_def;

};


// ----------------------------------------------------------------------------
//                  ParamsModelNode: a node inside ParamsModel
// ----------------------------------------------------------------------------

class ParamsNode;
using ParamsNodePtrArray = std::vector<std::unique_ptr<ParamsNode>>;

enum class ParamType {
    Undef,
    Scalar,
    Vector,
    FilamentVector,
};

// On all of 3 different platforms Bitmap+Text icon column looks different 
// because of Markup text is missed or not implemented.
// As a temporary workaround, we will use:
// MSW - DataViewBitmapText (our custom renderer wxBitmap + wxString, supported Markup text)
// OSX - -//-, but Markup text is not implemented right now
// GTK - wxDataViewIconText (wxWidgets for GTK renderer wxIcon + wxString, supported Markup text)
class ParamsNode
{
public:
    // Group params(root) node
    ParamsNode(const wxString& group_name, const std::string& icon_name);

    // sub SlicingState node
    ParamsNode(ParamsNode*          parent,
               const wxString&      sub_group_name,
               const std::string&   icon_name);

    // parametre node
    ParamsNode( ParamsNode*         parent, 
                ParamType           param_type,
                const std::string&  param_key);

    bool             IsContainer()      const           { return m_container; }
    bool             IsGroupNode()      const           { return m_parent == nullptr; }
    bool             IsParamNode()      const           { return m_param_type != ParamType::Undef; }
    void             SetContainer(bool is_container)    { m_container = is_container; }

    ParamsNode* GetParent() { return m_parent; }
    ParamsNodePtrArray& GetChildren() { return m_children; }

    void Append(std::unique_ptr<ParamsNode> child) { m_children.emplace_back(std::move(child)); }

public:

#ifdef __linux__
    wxIcon          icon;
#else
    wxBitmap        icon;
#endif //__linux__
    std::string     icon_name;
    std::string     param_key;
    wxString        text;

private:
    ParamsNode*         m_parent{ nullptr };
    ParamsNodePtrArray  m_children;
    ParamType           m_param_type{ ParamType::Undef };

    // TODO/FIXME:
    // the GTK version of wxDVC (in particular wxDataViewCtrlInternal::ItemAdded)
    // needs to know in advance if a node is or _will be_ a container.
    // Thus implementing:
    //   bool IsContainer() const
    //    { return m_children.size()>0; }
    // doesn't work with wxGTK when DiffModel::AddToClassical is called
    // AND the classical node was removed (a new node temporary without children
    // would be added to the control)
    bool                m_container{ true };
};


// ----------------------------------------------------------------------------
//                  ParamsModel
// ----------------------------------------------------------------------------

class ParamsModel : public wxDataViewModel
{
public:
    ParamsModel();
    ~ParamsModel() override = default;

    void            SetAssociatedControl(wxDataViewCtrl* ctrl) { m_ctrl = ctrl; }

    wxDataViewItem  AppendGroup(const wxString&    group_name,
                               const std::string& icon_name);

    wxDataViewItem  AppendSubGroup(wxDataViewItem    parent,
                                  const wxString&   sub_group_name,
                                  const std::string&icon_name);

    wxDataViewItem  AppendParam( wxDataViewItem      parent,
                                ParamType           param_type,
                                const std::string&  param_key);

    wxDataViewItem  Delete(const wxDataViewItem& item);

    wxString        GetParamName(wxDataViewItem item);
    std::string     GetParamKey(wxDataViewItem item);

    void            Clear();

    wxDataViewItem  GetParent(const wxDataViewItem& item) const override;
    unsigned int    GetChildren(const wxDataViewItem& parent, wxDataViewItemArray& array) const override;

    void            GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
    bool            SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;

    bool            IsContainer(const wxDataViewItem& item) const override;
    // Is the container just a header or an item with all columns
    // In our case it is an item with all columns
    bool            HasContainerColumns(const wxDataViewItem& WXUNUSED(item)) const override { return true; }

private:

    ParamsNodePtrArray  m_group_nodes;
    wxDataViewCtrl*     m_ctrl          { nullptr };

};


// ----------------------------------------------------------------------------
//                  ParamsViewCtrl
// ----------------------------------------------------------------------------

class ParamsViewCtrl : public wxDataViewCtrl
{
public:
    ParamsViewCtrl(wxWindow* parent, wxSize size);
    ~ParamsViewCtrl() override {
        if (model) {
            Clear();
            model->DecRef();
        }
    }

    wxDataViewItem  AppendGroup(const wxString&    group_name,
                                const std::string& icon_name);

    wxDataViewItem  AppendSubGroup(wxDataViewItem    parent,
                                   const wxString&   sub_group_name,
                                   const std::string&icon_name);

    wxDataViewItem  AppendParam(wxDataViewItem      parent,
                                ParamType           param_type,
                                const std::string&  param_key);

    wxString        GetValue(wxDataViewItem item);
    wxString        GetSelectedValue();
    std::string     GetSelectedParamKey();

    void    CheckAndDeleteIfEmpty(wxDataViewItem item);
    void    Clear();
    void    Rescale(int em = 0);
    void    set_em_unit(int em) { m_em_unit = em; }

public:
    ParamsModel*    model       { nullptr };

private:
    int     m_em_unit;
};


} 


