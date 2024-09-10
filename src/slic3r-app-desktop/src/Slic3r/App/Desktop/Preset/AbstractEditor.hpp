///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Enrico Turri @enricoturri1966, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2019 John Drake @foxox
///|/ Copyright (c) 2018 Martin Loidl @LoidlM
///|/
///|/ ported from lib/Slic3r/GUI/Tab.pm:
///|/ Copyright (c) Prusa Research 2016 - 2018 Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena
///|/ Copyright (c) 2015 - 2017 Joseph Lenox @lordofhyphens
///|/ Copyright (c) Slic3r 2012 - 2016 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2016 Chow Loong Jin @hyperair
///|/ Copyright (c) 2012 QuantumConcepts
///|/ Copyright (c) 2012 Henrik Brix Andersen @henrikbrixandersen
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

//     The "Expert" preset editor int tab at the right of the main tabbed window.
//    
//     This file implements following packages:
//       Slic3r::App::Desktop::Preset::AbstractEditor;
//           Slic3r::App::Desktop::Preset::AbstractEditor::EditorPrint;
//           Slic3r::App::Desktop::Preset::AbstractEditor::EditorFilament;
//           Slic3r::App::Desktop::Preset::AbstractEditor::EditorPrinter;
//           Slic3r::App::Desktop::Preset::AbstractEditor::EditorPrintSLA;
//           Slic3r::App::Desktop::Preset::AbstractEditor::EditorMaterialSLA;
//       Slic3r::App::Desktop::Preset::AbstractEditor::Page
//           - Option page: For example, the Slic3r::App::Desktop::Preset::Editor::Print has option pages "Layers and perimeters", "Infill", "Skirt and brim" ...

#include <wx/panel.h>

#include <map>
#include <vector>
#include <memory>

#include "EditorPage.hpp"
#include "../Config/OptionsGroup.hpp"

#include "Slic3r/Biz/Preset/PresetInteractorConfigContainerContext.hpp"
#include "Slic3r/Biz/Preset/PresetState.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

#include "../GUI_Descriptions.hpp"
#include "Slic3r/App/WX/Highlighter.hpp"
#include "Slic3r/App/WX/Scalable.hpp"
#include "Slic3r/App/WX/ConfigManipulation.hpp"

#include "libslic3r/Preset.hpp"

class wxKeyEvent;
class wxTreeCtrl;
class wxWindow;
class wxString;

namespace Slic3r::App::Desktop::Preset {

class EditorPresetComboBox;
class Manipulators;

struct PresetDependencies {
    Slic3r::Preset::Type    type{ Slic3r::Preset::TYPE_INVALID };
    wxWindow*               checkbox{ nullptr };
    WX::ScalableButton*     btn{ nullptr };
    std::string             key_list; // "compatible_printers"
    std::string             key_condition;
    wxString                dialog_title;
    wxString                dialog_label;

    PresetDependencies(Slic3r::Preset::Type type);
};

using namespace Config;

using PageShp = std::shared_ptr<Page>;
class AbstractEditor : public wxPanel
{
public:
    AbstractEditor(wxWindow* parent, const wxString& title, Slic3r::Preset::Type type);
    ~AbstractEditor() {}

    static wxString             translate_category(const wxString& title, Slic3r::Preset::Type preset_type);

    wxWindow*                   parent() const   { return m_parent; }
    wxString                    title()  const   { return m_title; }
    Slic3r::Preset::Type        type()   const   { return m_type; }
    DynamicPrintConfig*         config() const   { return m_config; }
    Biz::Preset::PresetState*   state()  const   { return m_state; }

    // The tab is already constructed.
    bool    completed() const { return m_completed; }

    void    init(Biz::Preset::PresetInteractorConfigContainerContext* ccc, 
                 Biz::Preset::PresetInteractor* preset_interactor,
                 PresetBundle* preset_bundle);
    void    update(Biz::Preset::PresetInteractorConfigContainerContext* ccc);
    void    activate();
    void    activate_option(const std::string& opt_key, const wxString& category);
    void    update_mode(ConfigOptionMode mode);
    void    update_mode_markers();
    void    update_label_colours();
    bool    validate_custom_gcodes();
    void    update_dirty();
    void    update_changed_tree_ui();
    void    on_value_change(const std::string& opt_key, const boost::any& value);

    virtual bool    supports_printer_technology(const PrinterTechnology tech) const { return true; }
    virtual void    msw_rescale();
    virtual void    sys_color_changed();

    const std::map<wxString, std::string>& get_category_icon_map() { return m_category_icon; }

    void    update_preset_choice();//?
    void    cache_config_diff(const std::vector<std::string>& selected_options, const DynamicPrintConfig* config = nullptr);//?
    void    apply_config_from_cache();//?

protected:
    PageShp         add_options_page(const wxString& title, const std::string& icon, bool is_extruder_pages = false);

    virtual void    load_current_preset();
    virtual void    clear_pages();
    virtual void    update_description_lines();
    virtual void    activate_selected_page(std::function<void()> throw_if_canceled);

    virtual void    on_preset_loaded() {}
    virtual void    build() = 0;
    virtual void    update() = 0;
    virtual void    toggle_options() = 0;
    virtual void    init_options_list();
    void            load_initial_data();
    void            update_tab_ui();
    void            load_config(const DynamicPrintConfig& config);
    virtual void    reload_config();
    virtual void    update_sla_prusa_specific_visibility() {}

    wxSizer*        description_line_widget(wxWindow* parent, ogStaticText** StaticText, wxString text = wxEmptyString);
    static bool     validate_custom_gcode(const wxString& title, const std::string& gcode);
    void            validate_custom_gcode_cb(const wxString& title, const t_config_option_key& opt_key, const boost::any& value);

    void                        edit_custom_gcode(const t_config_option_key& opt_key);
    virtual const std::string&  get_custom_gcode(const t_config_option_key& opt_key);
    virtual void                set_custom_gcode(const t_config_option_key& opt_key, const std::string& value);

    void    rebuild_page_tree();
    void    update_changed_ui();
    void    toggle_option(const std::string& opt_key, bool toggle, int opt_index = -1);
    bool    is_prusa_printer() const;

    void    create_legend(PageShp page, const std::vector<std::pair<std::string, std::string>>& columns, ConfigOptionMode mode, bool is_wider = false);
    std::optional<ConfigOptionsGroupShp> get_option_group(const Page* page, const std::string& title);
    void    add_options_into_line(ConfigOptionsGroupShp& optgroup,
                                  const std::vector<SamePair<std::string>>& prefixes,
                                  const std::string& optkey,
                                  const std::string& preprefix = std::string());

    void        create_line_with_widget(ConfigOptionsGroup* optgroup, const std::string& opt_key, const std::string& path, widget_t widget);
    wxSizer*    compatible_widget_create(wxWindow* parent, PresetDependencies &deps);
    void        compatible_widget_reload(PresetDependencies &deps);
    void        build_preset_description_line(ConfigOptionsGroup* optgroup);
    void        load_key_value(const std::string& opt_key, const boost::any& value, bool saved_value = false);

private:

    void    OnKeyDown(wxKeyEvent& event);
    void    emplace_option(const std::string &opt_key, bool respect_vec_values = false);

    void    add_scaled_button(wxWindow* parent, WX::ScalableButton** btn, const std::string& icon_name,
                              const wxString& label = wxEmptyString, 
                              long style = wxBU_EXACTFIT | wxNO_BORDER);
    void    add_scaled_bitmap(wxWindow* parent, WX::ScalableBitmap& btn, const std::string& icon_name);
    void    update_ui_items_related_on_parent_preset(const Slic3r::Preset* selected_preset_parent);
    void    fill_icon_descriptions();
    void    set_tooltips_text();


    // return true if cancelled
    bool    tree_sel_change_delayed();

    void    update_visibility();
    void    update_preset_description_line();
    void    decorate();
    void    get_sys_and_mod_flags(const std::string& opt_key, bool& sys_page, bool& modified_page);
    void    on_roll_back_value(const bool to_sys = false);
    void    update_undo_buttons();

    Field*  get_field(const t_config_option_key& opt_key, int opt_index = -1) const;
    Field*  get_field(const t_config_option_key &opt_key, Page** selected_page, int opt_index = -1);
    Line*   get_line(const t_config_option_key& opt_key);
    WX::ConfigManipulation      get_config_manipulation();
    std::pair<wxPanel*, bool*>  get_custom_ctrl_with_blinking_ptr(const t_config_option_key& opt_key, int opt_index = -1);

protected:

    Slic3r::Preset::Type    m_type;
    std::string             m_name;
    const wxString          m_title;

    Biz::Preset::PresetInteractorConfigContainerContext*    m_ccc       { nullptr };
    Biz::Preset::PresetState*                               m_state     { nullptr };
    DynamicPrintConfig*                                     m_config    { nullptr };

    ConfigOptionMode        m_mode                  { comExpert }; // to correct first Tab update_visibility() set mode to Expert
    WX::ConfigManipulation  m_config_manipulation;
    std::vector<PageShp>    m_pages;
    Page*                   m_active_page           {nullptr};

    EditorPresetComboBox*   m_presets_choice        {nullptr};
    Manipulators*           m_manipulators          {nullptr};

    PresetDependencies      m_compatible_printers   { PresetDependencies(Slic3r::Preset::TYPE_PRINTER) };
    PresetDependencies      m_compatible_prints     { PresetDependencies(Slic3r::Preset::TYPE_PRINT) };

    enum OptStatus { 
        osSystemValue = 1, 
        osInitValue = 2 
    };
    std::map<std::string, int>  m_options_list;
    int                         m_opt_status_value = 0;

    /* Indicates, that default preset or preset inherited from default is selected
     * This value is used for a options color updating 
     * (use green color only for options, which values are equal to system values)
     */
    bool    m_is_default_preset                 { false };//?

    bool    m_validate_custom_gcodes_was_shown  { false };
    bool    m_page_switch_running               { false };
    bool    m_page_switch_planned               { false };

    // Counter for the updating (because of an update() function can have a recursive behavior):
    // 1. increase value from the very beginning of an update() function
    // 2. decrease value at the end of an update() function
    // 3. propagate changed configuration to the Plater when (m_update_cnt == 0) only
    int     m_update_cnt                        { 0 };

private:
    wxWindow*   m_parent;
#ifdef __WXOSX__
    wxPanel*    m_tmp_panel;
    int         m_size_move = -1;
#endif // __WXOSX__
    wxBoxSizer*         m_top_hsizer        {nullptr};
    wxBoxSizer*         m_hsizer            {nullptr};
    wxBoxSizer*         m_left_sizer        {nullptr};
    wxTreeCtrl*         m_treectrl          {nullptr};
    wxBoxSizer*         m_page_sizer        {nullptr};
    wxScrolledWindow*   m_page_view         {nullptr};

    WX::ScalableButton* m_undo_btn          {nullptr};
    WX::ScalableButton* m_undo_to_sys_btn   {nullptr};
    WX::ScalableButton* m_question_btn      {nullptr};

    // Bitmaps to be shown on the "Revert to system" aka "Lock to system" button next to each input field.
    WX::ScalableBitmap  m_bmp_value_lock;
    WX::ScalableBitmap  m_bmp_value_unlock;
    WX::ScalableBitmap  m_bmp_white_bullet;
    // The following bitmap points to either m_bmp_value_unlock or m_bmp_white_bullet, depending on whether the current preset has a parent preset.
    WX::ScalableBitmap  *m_bmp_non_system   {nullptr};
    // Bitmaps to be shown on the "Undo user changes" button next to each input field.
    WX::ScalableBitmap  m_bmp_value_revert;
    // Bitmaps to be shown on the "Undo user changes" button next to each input field.
    WX::ScalableBitmap  m_bmp_edit_value;
    
    std::vector<WX::ScalableButton*>    m_scaled_buttons = {};    
    std::vector<WX::ScalableBitmap*>    m_scaled_bitmaps = {};    
    std::vector<WX::ScalableBitmap>     m_scaled_icons_list = {};

    std::vector<GUI_Descriptions::ButtonEntry>  m_icon_descriptions = {};

    // Colors for ui "decoration"
    wxColour    m_sys_label_clr;
    wxColour    m_modified_label_clr;
    wxColour    m_default_text_clr;

    // Tooltip text for reset buttons (for whole options group)
    wxString    m_ttg_value_lock;
    wxString    m_ttg_value_unlock;
    wxString    m_ttg_white_bullet_ns;
    // The following text points to either m_ttg_value_unlock or m_ttg_white_bullet_ns, depending on whether the current preset has a parent preset.
    wxString    *m_ttg_non_system   {nullptr};
    // Tooltip text to be shown on the "Undo user changes" button next to each input field.
    wxString    m_ttg_white_bullet;
    wxString    m_ttg_value_revert;

    // Tooltip text for reset buttons (for each option in group)
    wxString    m_tt_value_lock;
    wxString    m_tt_value_unlock;
    // The following text points to either m_tt_value_unlock or m_ttg_white_bullet_ns, depending on whether the current preset has a parent preset.
    wxString    *m_tt_non_system   {nullptr};
    // Tooltip text to be shown on the "Undo user changes" button next to each input field.
    wxString    m_tt_white_bullet;
    wxString    m_tt_value_revert;

    int                             m_icon_count;
    std::map<std::string, size_t>   m_icon_index;        // Map from an icon file name to its index
    std::map<wxString, std::string> m_category_icon;    // Map from a category name to an icon file name

    bool    m_is_modified_values{ false };
    bool    m_is_nonsys_values{ true };
    bool    m_postpone_update_ui {false};

    bool    m_disable_tree_sel_changed_event {false};

    // To avoid actions with no-completed Tab
    bool    m_completed { false };
    int     m_em_unit;

    ogStaticText*           m_parent_preset_description_line    { nullptr };
    WX::HighlighterForWx    m_highlighter;
    DynamicPrintConfig      m_cache_config;
};

} 

