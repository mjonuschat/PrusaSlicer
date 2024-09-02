///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, Lukáš Hejl @hejllukas, Vojtěch Bubník @bubnikv, Pavel Mikuš @Godrak, Tomáš Mészáros @tamasmeszaros, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2021 Martin Budden
///|/ Copyright (c) 2021 Ilya @xorza
///|/ Copyright (c) 2019 John Drake @foxox
///|/ Copyright (c) 2019 Matthias Urlichs @smurfix
///|/ Copyright (c) 2019 Thomas Moore
///|/ Copyright (c) 2019 Sijmen Schoon
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


#include "AbstractEditor.hpp"
#include "EditorPrinter.hpp"
#include "EditorPresetComboBox.hpp"
#include "Manipulators.hpp"
#include "../Config/OptionsGroup.hpp"
#include "../Config/OG_CustomCtrl.hpp"
#include "EditGCodeDialog.hpp"

#include "Slic3r/Biz/Preset/PresetHints.hpp"

#include "libslic3r/GCode/GCodeProcessor.hpp"

#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/BitmapGetters.hpp"
#include "Slic3r/App/WX/MsgDialog.hpp"
#include "Slic3r/App/WX/format.hpp"

#include "Slic3r/App/WX/Widgets/CheckBox.hpp"

#include <wx/button.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/string.h>

#include <wx/bmpbuttn.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <wx/settings.h>
#include <wx/wupdlock.h>
#include <wx/bookctrl.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

//!#include "GUI_App.hpp"       for -> update_label_colours
//!#include "MainFrame.hpp"     for -> select_tab()
//!#include "AppConfig.hpp" 

//!#include "I18N.hpp"
#define _u8L(s) s
#define L(s) s
#define _(s) s
static wxString _L(const wxString& s) { return s; };
static wxString _L_PLURAL(const wxString& s1, const wxString& s2, int n) { return s1; };

#ifdef WIN32
	#include <CommCtrl.h>
#endif // WIN32

using namespace Slic3r::App::Desktop::Config;

namespace Slic3r::App::Desktop::Preset {

using WX::from_u8;
using WX::into_u8;
using WX::dots;

PresetDependencies::PresetDependencies(Slic3r::Preset::Type type):
    type(type)
{
    if (type == Slic3r::Preset::TYPE_PRINTER) {
        key_list		= "compatible_printers";
        key_condition	= "compatible_printers_condition";
        dialog_title    = _L("Compatible printers");
        dialog_label    = _L("Select the printers this profile is compatible with.");
    }

    if (type == Slic3r::Preset::TYPE_PRINT) {
        key_list 		= "compatible_prints";
        key_condition	= "compatible_prints_condition";
        dialog_title 	= _L("Compatible print profiles");
        dialog_label 	= _L("Select the print profiles this profile is compatible with.");
    }
}

AbstractEditor::AbstractEditor(wxWindow* parent, const wxString& title, Slic3r::Preset::Type type) :
    m_parent(parent), m_type(type), m_title(title)
{
    Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, /*wxBK_LEFT | */wxTAB_TRAVERSAL/*, name*/);
    this->SetFont(WX::w_config()->normal_font());

#ifdef __WXMSW__
    WX::w_config()->UpdateDarkUI(this);
#elif __WXOSX__
    SetBackgroundColour(parent->GetBackgroundColour());
#elif __WXGTK3__
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif

    m_em_unit = WX::w_config()->em_unit(m_parent);

    m_config_manipulation = get_config_manipulation();
    m_highlighter.set_timer_owner(this, 0);
}

// sub new
void AbstractEditor::init(  Biz::Preset::PresetInteractorConfigContainerContext* ccc, 
                            Biz::Preset::PresetInteractor* preset_interactor,
                            PresetBundle* preset_bundle)
{
    m_ccc = ccc;
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__

    // Vertical sizer to hold the choice menu and the rest of the page.
#ifdef __WXOSX__
    auto  *main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->SetSizeHints(this);
    this->SetSizer(main_sizer);

    // Create additional panel to Fit() it from activate()
    // It's needed for tooltip showing on OSX
    m_tmp_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL);
    auto panel = m_tmp_panel;
    auto  sizer = new wxBoxSizer(wxVERTICAL);
    m_tmp_panel->SetSizer(sizer);
    m_tmp_panel->Layout();

    main_sizer->Add(m_tmp_panel, 1, wxEXPAND | wxALL, 0);
#else
    AbstractEditor* panel = this;
    auto  *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->SetSizeHints(panel);
    panel->SetSizer(sizer);
#endif //__WXOSX__

    // preset chooser
    m_presets_choice = new EditorPresetComboBox(panel, m_type, preset_bundle);

    // set of buttons for maniplation(save/rename/delete...) with presets
    m_manipulators = new Manipulators(panel, m_presets_choice, preset_interactor);
    m_manipulators->show_btn_incompatible_presets();

    add_scaled_button(panel, &m_question_btn, "question");
    m_question_btn->SetToolTip(_L("Hover the cursor over buttons to find more information \n"
                                   "or click this button."));

    // Bitmaps to be shown on the "Revert to system" aka "Lock to system" button next to each input field.
    add_scaled_bitmap(this, m_bmp_value_lock  , "lock_closed");
    add_scaled_bitmap(this, m_bmp_value_unlock, "lock_open");
    m_bmp_non_system = &m_bmp_white_bullet;
    // Bitmaps to be shown on the "Undo user changes" button next to each input field.
    add_scaled_bitmap(this, m_bmp_value_revert, "undo");
    add_scaled_bitmap(this, m_bmp_white_bullet, "dot");
    // Bitmap to be shown on the "edit" button before to each editable input field.
    add_scaled_bitmap(this, m_bmp_edit_value, "edit");

    fill_icon_descriptions();
    set_tooltips_text();

    add_scaled_button(panel, &m_undo_btn,        m_bmp_white_bullet.name());
    add_scaled_button(panel, &m_undo_to_sys_btn, m_bmp_white_bullet.name());

    m_undo_btn->Bind(wxEVT_BUTTON, ([this](wxCommandEvent) { on_roll_back_value(); }));
    m_undo_to_sys_btn->Bind(wxEVT_BUTTON, ([this](wxCommandEvent) { on_roll_back_value(true); }));
    m_question_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent) {
        GUI_Descriptions::Dialog dlg(this, m_icon_descriptions);
        if (dlg.ShowModal() == wxID_OK)
            update_label_colours(); //! wxGetApp().update_label_colours();
    });

    // Colors for ui "decoration"
    m_sys_label_clr			= WX::w_config()->get_label_clr_sys();
    m_modified_label_clr	= WX::w_config()->get_label_clr_modified();
    m_default_text_clr		= WX::w_config()->get_label_clr_default();

    const float scale_factor = WX::w_config()->em_unit(this)*0.1;// GetContentScaleFactor();
    m_top_hsizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_top_hsizer, 0, wxEXPAND | wxBOTTOM, 3);
    m_top_hsizer->Add(m_presets_choice, 0, wxLEFT | wxRIGHT | wxTOP | wxALIGN_CENTER_VERTICAL, 3);
    m_top_hsizer->AddSpacer(int(4*scale_factor));
    m_top_hsizer->Add((wxBoxSizer*)m_manipulators, 0, wxLEFT | wxRIGHT | wxTOP | wxALIGN_CENTER_VERTICAL, 3);

    m_top_hsizer->AddSpacer(int(16*scale_factor));
    // StretchSpacer has a strange behavior under OSX, so
    // hide whole top sizer to correct layout later
//!    m_top_hsizer->ShowItems(false);

    //Horizontal sizer to hold the tree and the selected page.
    m_hsizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_hsizer, 1, wxEXPAND, 0);

    //left vertical sizer
    m_left_sizer = new wxBoxSizer(wxVERTICAL);
    m_hsizer->Add(m_left_sizer, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 3);

    // tree
    m_treectrl = new wxTreeCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(20 * m_em_unit, -1),
        wxTR_NO_BUTTONS | wxTR_HIDE_ROOT | wxTR_SINGLE | wxTR_NO_LINES | wxBORDER_SUNKEN | wxWANTS_CHARS);
    m_treectrl->SetFont(WX::w_config()->normal_font());
#ifdef __linux__
    m_treectrl->SetBackgroundColour(m_parent->GetBackgroundColour());
#endif
    m_left_sizer->Add(m_treectrl, 1, wxEXPAND);
    // Index of the last icon inserted into m_treectrl
    m_icon_count = -1;
    m_treectrl->AddRoot("root");
    m_treectrl->SetIndent(0);
    WX::w_config()->UpdateDarkUI(m_treectrl);

    // Delay processing of the following handler until the message queue is flushed.
    // This helps to process all the cursor key events on Windows in the tree control,
    // so that the cursor jumps to the last item.
    m_treectrl->Bind(wxEVT_TREE_SEL_CHANGED, [this](wxTreeEvent&) {
#ifdef __linux__
        // Events queue is opposite On Linux. wxEVT_SET_FOCUS invokes after wxEVT_TREE_SEL_CHANGED,
        // and a result wxEVT_KILL_FOCUS doesn't invoke for the TextCtrls.
        // see https://github.com/prusa3d/PrusaSlicer/issues/5720
        // So, call SetFocus explicitly for this control before changing of the selection
        m_treectrl->SetFocus();
#endif
            if (!m_disable_tree_sel_changed_event && !m_pages.empty()) {
                if (m_page_switch_running)
                    m_page_switch_planned = true;
                else {
                    m_page_switch_running = true;
                    do {
                        m_page_switch_planned = false;
                        m_treectrl->Update();
                    } while (this->tree_sel_change_delayed());
                    m_page_switch_running = false;
                }
            }
        });

    m_treectrl->Bind(wxEVT_KEY_DOWN, &AbstractEditor::OnKeyDown, this);

    // Initialize the page.
#ifdef __WXOSX__
    auto page_parent = m_tmp_panel;
#else
    auto page_parent = this;
#endif

    m_page_view = new wxScrolledWindow(page_parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_page_sizer = new wxBoxSizer(wxVERTICAL);
    m_page_view->SetSizer(m_page_sizer);
    m_page_view->SetScrollbars(1, 20, 1, 2);

#if 0
    m_hsizer->Add(m_page_view, 1, wxEXPAND | wxLEFT, 5);
#else

    wxBoxSizer* act_bnts_sizer = new wxBoxSizer(wxHORIZONTAL);

    act_bnts_sizer->AddSpacer(int(25 * m_em_unit));
    act_bnts_sizer->Add(m_undo_to_sys_btn, 0, wxALIGN_CENTER_VERTICAL);
    act_bnts_sizer->Add(m_undo_btn, 0, wxALIGN_CENTER_VERTICAL);
    act_bnts_sizer->AddSpacer(int(5 * m_em_unit));
    act_bnts_sizer->Add(m_question_btn, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* right_sizer = new wxBoxSizer(wxVERTICAL);
    right_sizer->Add(act_bnts_sizer);
    right_sizer->Add(m_page_view, 1, wxEXPAND | wxTOP, 5);
    
    m_hsizer->Add(right_sizer, 1, wxEXPAND | wxLEFT, 5);
#endif

    // Initialize the DynamicPrintConfig by default keys/values.
    build();

    if (!m_scaled_icons_list.empty()) {
        // update icons for tree_ctrl
        wxVector<wxBitmapBundle> img_bundles;
        for (const WX::ScalableBitmap& bmp : m_scaled_icons_list)
            img_bundles.push_back(bmp.bmp());
        m_treectrl->SetImages(img_bundles);
    }

    // ys_FIXME: Following should not be needed, the function will be called later
    // (update_mode->update_visibility->rebuild_page_tree). This does not work, during the
    // second call of rebuild_page_tree m_treectrl->GetFirstVisibleItem(); returns zero
    // for some unknown reason (and the page is not refreshed until user does a selection).
    rebuild_page_tree();

    m_completed = true;
}

void AbstractEditor::update(Biz::Preset::PresetInteractorConfigContainerContext* ccc)
{
    m_ccc = ccc;

    if (m_type == Slic3r::Preset::TYPE_PRINT || m_type == Slic3r::Preset::TYPE_SLA_PRINT)
        m_state = &m_ccc->print;
    else if (m_type == Slic3r::Preset::TYPE_FILAMENT || m_type == Slic3r::Preset::TYPE_SLA_MATERIAL)
        m_state = &m_ccc->materials[0];//! ysFIXME -> set correct extruder id
    else
        m_state = &m_ccc->printer;

    m_config = &m_state->edited_preset.config;

    // update presets list and manipulators from the state
    m_presets_choice->update(m_state, &m_ccc->preset_bundle_runtime);
    m_manipulators->update(m_state, m_ccc->printer.edited_preset.config.opt_string("printer_model"), m_ccc->ph_printer_name);

    reload_config();
}

void AbstractEditor::add_scaled_button(wxWindow* parent,
                            WX::ScalableButton** btn,
                            const std::string& icon_name,
                            const wxString& label/* = wxEmptyString*/,
                            long style /*= wxBU_EXACTFIT | wxNO_BORDER*/)
{
    *btn = new WX::ScalableButton(parent, wxID_ANY, icon_name, label, wxDefaultSize, wxDefaultPosition, style);
    m_scaled_buttons.push_back(*btn);
}

void AbstractEditor::add_scaled_bitmap(wxWindow* parent,
                            WX::ScalableBitmap& bmp,
                            const std::string& icon_name)
{
    bmp = WX::ScalableBitmap(parent, icon_name);
    m_scaled_bitmaps.push_back(&bmp);
}

void AbstractEditor::load_initial_data()
{
    m_config = &m_state->edited_preset.config;
    bool has_parent = m_state->selected_preset_parent != nullptr;
    m_bmp_non_system = has_parent ? &m_bmp_value_unlock : &m_bmp_white_bullet;
    m_ttg_non_system = has_parent ? &m_ttg_value_unlock : &m_ttg_white_bullet_ns;
    m_tt_non_system  = has_parent ? &m_tt_value_unlock  : &m_ttg_white_bullet_ns;
}

PageShp AbstractEditor::add_options_page(const wxString& title, const std::string& icon, bool is_extruder_pages /*= false*/)
{
    // Index of icon in an icon list $self->{icons}.
    auto icon_idx = 0;
    if (!icon.empty()) {
        icon_idx = (m_icon_index.find(icon) == m_icon_index.end()) ? -1 : m_icon_index.at(icon);
        if (icon_idx == -1) {
            // Add a new icon to the icon list.
            m_scaled_icons_list.push_back(WX::ScalableBitmap(this, icon));
            icon_idx = ++m_icon_count;
            m_icon_index[icon] = icon_idx;
        }

        if (m_category_icon.find(title) == m_category_icon.end()) {
            // Add new category to the category_to_icon list.
            m_category_icon[title] = icon;
        }
    }
    // Initialize the page.
    PageShp page(new Page(m_page_view, title, icon_idx));

    if (!is_extruder_pages)
        m_pages.push_back(page);

    page->set_config(m_config);
    return page;
}

// Names of categories is save in English always. We translate them only for UI.
// But category "Extruder n" can't be translated regularly (using _()), so
// just for this category we should splite the title and translate "Extruder" word separately
wxString AbstractEditor::translate_category(const wxString& title, Slic3r::Preset::Type preset_type)
{
    if (preset_type == Slic3r::Preset::TYPE_PRINTER && title.Contains("Extruder ")) {
        return _("Extruder") + title.SubString(8, title.Last());
    }
    return _(title);
}

void AbstractEditor::activate()
{
    load_current_preset(); //!new -> just for test

    wxWindowUpdateLocker noUpdates(this);
#ifdef __WXOSX__
//    wxWindowUpdateLocker noUpdates(this);
    auto size = GetSizer()->GetSize();
    m_tmp_panel->GetSizer()->SetMinSize(size.x + m_size_move, size.y);
    Fit();
    m_size_move *= -1;
#endif // __WXOSX__

#ifdef __WXMSW__
    // Workaround for tooltips over Tree Controls displayed over excessively long
    // tree control items, stealing the window focus.
    //
    // In case the Tab was reparented from the MainFrame to the floating dialog,
    // the tooltip created by the Tree Control before reparenting is not reparented, 
    // but it still points to the MainFrame. If the tooltip pops up, the MainFrame 
    // is incorrectly focussed, stealing focus from the floating dialog.
    //
    // The workaround is to delete the tooltip control.
    // Vojtech tried to reparent the tooltip control, but it did not work,
    // and if the Tab was later reparented back to MainFrame, the tooltip was displayed
    // at an incorrect position, therefore it is safer to just discard the tooltip control
    // altogether.
    HWND hwnd_tt = TreeView_GetToolTips(m_treectrl->GetHandle());
    if (hwnd_tt) {
	    HWND hwnd_toplevel 	= WX::w_config()->find_toplevel_parent(m_treectrl)->GetHandle();
	    HWND hwnd_parent 	= ::GetParent(hwnd_tt);
	    if (hwnd_parent != hwnd_toplevel) {
	    	::DestroyWindow(hwnd_tt);
			TreeView_SetToolTips(m_treectrl->GetHandle(), nullptr);
	    }
    }
#endif

    // create controls on active page
    activate_selected_page([](){});
    m_hsizer->Layout();

    if (m_question_btn->IsShown())
        Refresh(); // Just refresh page, if m_presets_choice is already shown
    else {
        // From the tab creation whole top sizer is hidden to correct update of preset combobox's size
        // (see https://github.com/prusa3d/PrusaSlicer/issues/10746)

        // On first OnActivate call show top sizer
        m_top_hsizer->ShowItems(true);

        //! update_extruder_combobox_visibility();

        Layout();
    }
}

void AbstractEditor::update_label_colours()
{
    m_default_text_clr = WX::w_config()->get_label_clr_default();
    if (m_sys_label_clr == WX::w_config()->get_label_clr_sys() && m_modified_label_clr == WX::w_config()->get_label_clr_modified())
        return;
    m_sys_label_clr = WX::w_config()->get_label_clr_sys();
    m_modified_label_clr = WX::w_config()->get_label_clr_modified();

    //update options "decoration"
    for (const auto& opt : m_options_list)
    {
        const wxColour *color = &m_sys_label_clr;

        // value isn't equal to system value
        if ((opt.second & osSystemValue) == 0) {
            // value is equal to last saved
            if ((opt.second & osInitValue) != 0)
                color = &m_default_text_clr;
            // value is modified
            else
                color = &m_modified_label_clr;
        }
        if (OptionsGroup::is_option_without_field(opt.first)) {
            if (Line* line = get_line(opt.first))
                line->set_label_colour(color);
            continue;
        }

        Field* field = get_field(opt.first);
        if (field == nullptr) continue;
        field->set_label_colour(color);
    }

    auto cur_item = m_treectrl->GetFirstVisibleItem();
    if (!cur_item || !m_treectrl->IsVisible(cur_item))
        return;
    while (cur_item) {
        auto title = m_treectrl->GetItemText(cur_item);
        for (auto page : m_pages)
        {
            if (translate_category(page->title(), m_type) != title)
                continue;

            const wxColor *clr = !page->is_nonsys_values ? &m_sys_label_clr :
                page->is_modified_values ? &m_modified_label_clr :
                &m_default_text_clr;

            m_treectrl->SetItemTextColour(cur_item, *clr);
            break;
        }
        cur_item = m_treectrl->GetNextVisible(cur_item);
    }

    decorate();
}

void AbstractEditor::decorate()
{
    for (const auto& opt : m_options_list)
    {
        Field*      field = nullptr;
        bool        option_without_field = false;

        if(OptionsGroup::is_option_without_field(opt.first))
            option_without_field = true;

        if (!option_without_field) {
            field = get_field(opt.first);
            if (!field)
                continue;
        }

        bool is_nonsys_value = false;
        bool is_modified_value = true;
        const WX::ScalableBitmap* sys_icon  = &m_bmp_value_lock;
        const WX::ScalableBitmap* icon      = &m_bmp_value_revert;

        const wxColour* color = m_is_default_preset ? &m_default_text_clr : &m_sys_label_clr;

        const wxString* sys_tt  = &m_tt_value_lock;
        const wxString* tt      = &m_tt_value_revert;

        // value isn't equal to system value
        if ((opt.second & osSystemValue) == 0) {
            is_nonsys_value = true;
            sys_icon = m_bmp_non_system;
            sys_tt = m_tt_non_system;
            // value is equal to last saved
            if ((opt.second & osInitValue) != 0)
                color = &m_default_text_clr;
            // value is modified
            else
                color = &m_modified_label_clr;
        }
        if ((opt.second & osInitValue) != 0)
        {
            is_modified_value = false;
            icon = &m_bmp_white_bullet;
            tt = &m_tt_white_bullet;
        }

        if (option_without_field) {
            if (Line* line = get_line(opt.first)) {
                line->set_undo_bitmap(icon);
                line->set_undo_to_sys_bitmap(sys_icon);
                line->set_undo_tooltip(tt);
                line->set_undo_to_sys_tooltip(sys_tt);
                line->set_label_colour(color);
            }
            continue;
        }
        
        field->m_is_nonsys_value = is_nonsys_value;
        field->m_is_modified_value = is_modified_value;
        field->set_undo_bitmap(icon);
        field->set_undo_to_sys_bitmap(sys_icon);
        field->set_undo_tooltip(tt);
        field->set_undo_to_sys_tooltip(sys_tt);
        field->set_label_colour(color);

        if (field->has_edit_ui())
            field->set_edit_bitmap(&m_bmp_edit_value);

    }

    if (m_active_page)
        m_active_page->refresh();
}

// Update UI according to changes
void AbstractEditor::update_changed_ui()
{
    if (m_postpone_update_ui)
        return;

    const bool deep_compare = m_type != Slic3r::Preset::TYPE_FILAMENT;
    auto dirty_options = m_state->current_dirty_options(deep_compare);
    auto nonsys_options = m_state->current_different_from_parent_options(deep_compare);
    if (m_type == Slic3r::Preset::TYPE_PRINTER) {
        {
            auto check_bed_custom_options = [](std::vector<std::string>& keys) {
                size_t old_keys_size = keys.size();
                keys.erase(std::remove_if(keys.begin(), keys.end(), [](const std::string& key) { 
                    return key == "bed_custom_texture" || key == "bed_custom_model"; }), keys.end());
                if (old_keys_size != keys.size() && std::find(keys.begin(), keys.end(), "bed_shape") == keys.end())
                    keys.emplace_back("bed_shape");
            };
            check_bed_custom_options(dirty_options);
            check_bed_custom_options(nonsys_options);
        }

        if (static_cast<EditorPrinter*>(this)->printer_technology == ptFFF) {
            EditorPrinter* tab = static_cast<EditorPrinter*>(this);
            if (tab->is_init_extruders_cnt_dirty())
                dirty_options.emplace_back("extruders_count");
            if (tab->is_sys_extruders_cnt_dirty())
                nonsys_options.emplace_back("extruders_count");
        }
    }

    for (auto& it : m_options_list)
        it.second = m_opt_status_value;

    for (auto opt_key : dirty_options)	m_options_list[opt_key] &= ~osInitValue;
    for (auto opt_key : nonsys_options)	m_options_list[opt_key] &= ~osSystemValue;

    decorate();

    wxTheApp->CallAfter([this]() {
        if (parent()) //To avoid a crash, parent should be exist for a moment of a tree updating
            update_changed_tree_ui();
    });
}

void AbstractEditor::init_options_list()
{
    m_options_list.clear();

    for (const std::string& opt_key : m_config->keys())
        emplace_option(opt_key, m_type != Slic3r::Preset::TYPE_FILAMENT && !PresetCollection::is_independent_from_extruder_number_option(opt_key));
}

template<class T>
void add_correct_opts_to_options_list(const std::string &opt_key, std::map<std::string, int>& map, AbstractEditor*tab, int value)
{
    T *opt_cur = static_cast<T*>(tab->config()->option(opt_key));
    for (size_t i = 0; i < opt_cur->values.size(); i++)
        map.emplace(opt_key + "#" + std::to_string(i), value);
}

void AbstractEditor::emplace_option(const std::string& opt_key, bool respect_vec_values/* = false*/)
{
    if (respect_vec_values) {
        switch (m_config->option(opt_key)->type())
        {
        case coInts:	add_correct_opts_to_options_list<ConfigOptionInts		>(opt_key, m_options_list, this, m_opt_status_value);	break;
        case coBools:	add_correct_opts_to_options_list<ConfigOptionBools		>(opt_key, m_options_list, this, m_opt_status_value);	break;
        case coFloats:	add_correct_opts_to_options_list<ConfigOptionFloats		>(opt_key, m_options_list, this, m_opt_status_value);	break;
        case coStrings:	add_correct_opts_to_options_list<ConfigOptionStrings	>(opt_key, m_options_list, this, m_opt_status_value);	break;
        case coPercents:add_correct_opts_to_options_list<ConfigOptionPercents	>(opt_key, m_options_list, this, m_opt_status_value);	break;
        case coPoints:	add_correct_opts_to_options_list<ConfigOptionPoints		>(opt_key, m_options_list, this, m_opt_status_value);	break;
        case coFloatsOrPercents:	add_correct_opts_to_options_list<ConfigOptionFloatsOrPercents		>(opt_key, m_options_list, this, m_opt_status_value);	break;
        case coEnums:	add_correct_opts_to_options_list<ConfigOptionEnumsGeneric>(opt_key, m_options_list, this, m_opt_status_value);	break;
        default:		m_options_list.emplace(opt_key, m_opt_status_value);		break;
        }
    }
    else 
        m_options_list.emplace(opt_key, m_opt_status_value);
}

void AbstractEditor::get_sys_and_mod_flags(const std::string& opt_key, bool& sys_page, bool& modified_page)
{
    auto opt = m_options_list.find(opt_key);
    if (opt == m_options_list.end()) 
        return;

    if (sys_page) sys_page = (opt->second & osSystemValue) != 0;
    modified_page |= (opt->second & osInitValue) == 0;
}

void AbstractEditor::update_changed_tree_ui()
{
    if (m_options_list.empty())
        return;
    auto cur_item = m_treectrl->GetFirstVisibleItem();
    if (!cur_item || !m_treectrl->IsVisible(cur_item))
        return;

    auto selected_item = m_treectrl->GetSelection();
    auto selection = selected_item ? m_treectrl->GetItemText(selected_item) : "";

    while (cur_item) {
        auto title = m_treectrl->GetItemText(cur_item);
        for (auto page : m_pages)
        {
            if (translate_category(page->title(), m_type) != title)
                continue;
            bool sys_page = true;
            bool modified_page = false;
            if (page->title() == "General") {
                std::initializer_list<const char*> optional_keys{ "extruders_count", "bed_shape" };
                for (auto &opt_key : optional_keys) {
                    get_sys_and_mod_flags(opt_key, sys_page, modified_page);
                }
            }
            if (m_type == Slic3r::Preset::TYPE_FILAMENT && page->title() == "Advanced") {
                get_sys_and_mod_flags("filament_ramming_parameters", sys_page, modified_page);
            }
            if (page->title() == "Dependencies") {
                if (m_type == Slic3r::Preset::TYPE_PRINTER) {
                    sys_page = m_state->selected_preset_parent != nullptr;
                    modified_page = false;
                } else {
                    if (m_type == Slic3r::Preset::TYPE_FILAMENT || m_type == Slic3r::Preset::TYPE_SLA_MATERIAL)
                        get_sys_and_mod_flags("compatible_prints", sys_page, modified_page);
                    get_sys_and_mod_flags("compatible_printers", sys_page, modified_page);
                }
            }
            for (auto group : page->optgroups)
            {
                if (!sys_page && modified_page)
                    break;
                for (const auto &kvp : group->opt_map()) {
                    const std::string& opt_key = kvp.first;
                    get_sys_and_mod_flags(opt_key, sys_page, modified_page);
                }
            }

            const wxColor *clr = sys_page		?	(m_is_default_preset ? &m_default_text_clr : &m_sys_label_clr) :
                                 modified_page	?	&m_modified_label_clr :
                                                    &m_default_text_clr;

            if (page->set_item_colour(clr))
                m_treectrl->SetItemTextColour(cur_item, *clr);

            page->is_nonsys_values = !sys_page;
            page->is_modified_values = modified_page;

            if (selection == title) {
                m_is_nonsys_values = page->is_nonsys_values;
                m_is_modified_values = page->is_modified_values;
            }
            break;
        }
        auto next_item = m_treectrl->GetNextVisible(cur_item);
        cur_item = next_item;
    }
    update_undo_buttons();
}

void AbstractEditor::update_undo_buttons()
{
    m_undo_btn->        SetBitmap_(m_is_modified_values ? m_bmp_value_revert.name(): m_bmp_white_bullet.name());
    m_undo_to_sys_btn-> SetBitmap_(m_is_nonsys_values   ? m_bmp_non_system->name() : m_bmp_value_lock.name());

    m_undo_btn->SetToolTip(m_is_modified_values ? m_ttg_value_revert : m_ttg_white_bullet);
    m_undo_to_sys_btn->SetToolTip(m_is_nonsys_values ? *m_ttg_non_system : m_ttg_value_lock);
}

void AbstractEditor::on_roll_back_value(const bool to_sys /*= true*/)
{
    if (!m_active_page) return;

    int os;
    if (to_sys)	{
        if (!m_is_nonsys_values) return;
        os = osSystemValue;
    }
    else {
        if (!m_is_modified_values) return;
        os = osInitValue;
    }

    m_postpone_update_ui = true;

    for (auto group : m_active_page->optgroups) {
        if (group->title == "Capabilities") {
            if ((m_options_list["extruders_count"] & os) == 0)
                to_sys ? group->back_to_sys_value("extruders_count") : group->back_to_initial_value("extruders_count");
        }
        if (group->title == "Size and coordinates") {
            if ((m_options_list["bed_shape"] & os) == 0) {
                to_sys ? group->back_to_sys_value("bed_shape") : group->back_to_initial_value("bed_shape");
                load_key_value("bed_shape", true/*some value*/, true);
            }
        }
        if (group->title == "Toolchange parameters with single extruder MM printers") {
            if ((m_options_list["filament_ramming_parameters"] & os) == 0)
                to_sys ? group->back_to_sys_value("filament_ramming_parameters") : group->back_to_initial_value("filament_ramming_parameters");
        }
        if (group->title == "G-code Substitutions") {
            if ((m_options_list["gcode_substitutions"] & os) == 0) {
                to_sys ? group->back_to_sys_value("gcode_substitutions") : group->back_to_initial_value("gcode_substitutions");
                load_key_value("gcode_substitutions", true/*some value*/, true);
            }
        }
        if (group->title == "Profile dependencies") {
            // "compatible_printers" option doesn't exists in Printer Settimgs Tab
            if (m_type != Slic3r::Preset::TYPE_PRINTER && (m_options_list["compatible_printers"] & os) == 0) {
                to_sys ? group->back_to_sys_value("compatible_printers") : group->back_to_initial_value("compatible_printers");
                load_key_value("compatible_printers", true/*some value*/, true);
            }
            // "compatible_prints" option exists only in Filament Settimgs and Materials Tabs
            if ((m_type == Slic3r::Preset::TYPE_FILAMENT || m_type == Slic3r::Preset::TYPE_SLA_MATERIAL) && (m_options_list["compatible_prints"] & os) == 0) {
                to_sys ? group->back_to_sys_value("compatible_prints") : group->back_to_initial_value("compatible_prints");
                load_key_value("compatible_prints", true/*some value*/, true);
            }
        }
        for (const auto &kvp : group->opt_map()) {
            const std::string& opt_key = kvp.first;
            if ((m_options_list[opt_key] & os) == 0)
                to_sys ? group->back_to_sys_value(opt_key) : group->back_to_initial_value(opt_key);
        }
    }

    m_postpone_update_ui = false;

    // When all values are rolled, then we have to update whole tab in respect to the reverted values
    update();

    update_changed_ui();
}

// Update the combo box label of the selected preset based on its "dirty" state,
// comparing the selected preset config with $self->{config}.
void AbstractEditor::update_dirty()
{
//!    m_presets_choice->update_dirty();
//!    on_presets_changed();
    update_changed_ui();
}

void AbstractEditor::update_tab_ui()
{
//!    m_presets_choice->update();
}

// Load a provied DynamicConfig into the tab, modifying the active preset.
// This could be used for example by setting a Wipe Tower position by interactive manipulation in the 3D view.
void AbstractEditor::load_config(const DynamicPrintConfig& config)
{
    bool modified = 0;
    for(auto opt_key : m_config->diff(config)) {
        m_config->set_key_value(opt_key, config.option(opt_key)->clone());
        modified = 1;
    }
    if (modified) {
        update_dirty();
        //# Initialize UI components with the config values.
        reload_config();
        update();
    }
}

// Reload current $self->{config} (aka $self->{presets}->edited_preset->config) into the UI fields.
void AbstractEditor::reload_config()
{
    if (m_active_page)
        m_active_page->reload_config();
}

void AbstractEditor::update_mode(ConfigOptionMode mode)
{
    m_mode = mode;

    update_visibility();
    update_sla_prusa_specific_visibility();

    update_changed_tree_ui();
}

void AbstractEditor::update_mode_markers()
{
    if (m_active_page)
        m_active_page->refresh();
}

void AbstractEditor::update_visibility()
{
    Freeze(); // There is needed Freeze/Thaw to avoid a flashing after Show/Layout

    for (auto page : m_pages)
        page->update_visibility(m_mode, page.get() == m_active_page);
    rebuild_page_tree();

    if (m_type != Slic3r::Preset::TYPE_PRINTER)
        update_description_lines();

    Layout();
    Thaw();
}

void AbstractEditor::msw_rescale()
{
    m_em_unit = WX::w_config()->em_unit(m_parent);

    m_presets_choice->msw_rescale();
    m_treectrl->SetMinSize(wxSize(20 * m_em_unit, -1));

    if (m_compatible_printers.checkbox)
        CheckBox::Rescale(m_compatible_printers.checkbox);
    if (m_compatible_prints.checkbox)
        CheckBox::Rescale(m_compatible_prints.checkbox);

    // rescale options_groups
    if (m_active_page)
        m_active_page->msw_rescale();

    Layout();
}

void AbstractEditor::sys_color_changed()
{
    m_presets_choice->sys_color_changed();
    m_manipulators->sys_color_changed();

    // update buttons and cached bitmaps
    for (const auto btn : m_scaled_buttons)
        btn->sys_color_changed();
    for (const auto bmp : m_scaled_bitmaps)
        bmp->sys_color_changed();

    // update icons for tree_ctrl
    wxVector <wxBitmapBundle> img_bundles;
    for (WX::ScalableBitmap& bmp : m_scaled_icons_list) {
        bmp.sys_color_changed();
        img_bundles.push_back(bmp.bmp());
    }
    m_treectrl->SetImages(img_bundles);

    // Colors for ui "decoration"
    update_label_colours();
#ifdef _WIN32
    wxWindowUpdateLocker noUpdates(this);
    WX::w_config()->UpdateDarkUI(this);
    WX::w_config()->UpdateDarkUI(m_treectrl);
#endif
    update_changed_tree_ui();

    // update options_groups
    if (m_active_page)
        m_active_page->sys_color_changed();

    Layout();
    Refresh();
}

Field* AbstractEditor::get_field(const t_config_option_key& opt_key, int opt_index/* = -1*/) const
{
    return m_active_page ? m_active_page->get_field(opt_key, opt_index) : nullptr;
}

Line* AbstractEditor::get_line(const t_config_option_key& opt_key)
{
    return m_active_page ? m_active_page->get_line(opt_key) : nullptr;
}

std::pair<wxPanel*, bool*> AbstractEditor::get_custom_ctrl_with_blinking_ptr(const t_config_option_key& opt_key, int opt_index/* = -1*/)
{
    if (!m_active_page)
        return {nullptr, nullptr};

    std::pair<wxPanel*, bool*> ret = {nullptr, nullptr};

    for (auto opt_group : m_active_page->optgroups) {
        ret = opt_group->get_custom_ctrl_with_blinking_ptr(opt_key, opt_index);
        if (ret.first && ret.second)
            break;
    }
    return ret;
}

Field* AbstractEditor::get_field(const t_config_option_key& opt_key, Page** selected_page, int opt_index/* = -1*/)
{
    Field* field = nullptr;
    for (auto page : m_pages) {
        field = page->get_field(opt_key, opt_index);
        if (field != nullptr) {
            *selected_page = page.get();
            return field;
        }
    }
    return field;
}

void AbstractEditor::toggle_option(const std::string& opt_key, bool toggle, int opt_index/* = -1*/)
{
    if (!m_active_page)
        return;
    Field* field = m_active_page->get_field(opt_key, opt_index);
    if (field)
        field->toggle(toggle);
};

// To be called by custom widgets, load a value into a config,
// update the preset selection boxes (the dirty flags)
// If value is saved before calling this function, put saved_value = true,
// and value can be some random value because in this case it will not been used
void AbstractEditor::load_key_value(const std::string& opt_key, const boost::any& value, bool saved_value /*= false*/)
{
    if (!saved_value) OptionsGroup::change_opt_value(*m_config, opt_key, value);
    // Mark the print & filament enabled if they are compatible with the currently selected preset.
    if (opt_key == "compatible_printers" || opt_key == "compatible_prints") {
        // Don't select another profile if this profile happens to become incompatible.
//!        m_preset_bundle->update_compatible(PresetSelectCompatibleType::Never);
    }
//!    m_presets_choice->update_dirty();
//!    on_presets_changed();
    update();
}

void AbstractEditor::on_value_change(const std::string& opt_key, const boost::any& value)
{
    if (opt_key == "compatible_prints")
        this->compatible_widget_reload(m_compatible_prints);
    if (opt_key == "compatible_printers")
        this->compatible_widget_reload(m_compatible_printers);

/*
    //! propagate changes to Sidebar

    if (wxGetApp().plater() == nullptr) {
        return;
    }

    const bool is_fff = supports_printer_technology(ptFFF);
    ConfigOptionsGroup* og_freq_chng_params = wxGetApp().sidebar().og_freq_chng_params(is_fff);
    if (opt_key == "fill_density" || opt_key == "pad_enable")
    {
        boost::any val = og_freq_chng_params->get_config_value(*m_config, opt_key);
        og_freq_chng_params->set_value(opt_key, val);
    }
    
    if (opt_key == "pad_around_object") {
        for (PageShp &pg : m_pages) {
            Field * fld = pg->get_field(opt_key); /// !!! ysFIXME ????
            if (fld) fld->set_value(value, false);
        }
    }

    if (is_fff ?
            (opt_key == "support_material" || opt_key == "support_material_auto" || opt_key == "support_material_buildplate_only") :
            (opt_key == "supports_enable"  || opt_key == "support_tree_type" || opt_key == get_sla_suptree_prefix(*m_config) + "support_buildplate_only" || opt_key == "support_enforcers_only"))
        og_freq_chng_params->set_value("support", support_combo_value_for_config(*m_config, is_fff));

    if (! is_fff && (opt_key == "pad_enable" || opt_key == "pad_around_object"))
        og_freq_chng_params->set_value("pad", pad_combo_value_for_config(*m_config));

    if (opt_key == "brim_width")
    {
        bool val = m_config->opt_float("brim_width") > 0.0 ? true : false;
        og_freq_chng_params->set_value("brim", val);
    }

    if (opt_key == "wipe_tower" || opt_key == "single_extruder_multi_material" || opt_key == "extruders_count" )
        update_wiping_button_visibility();

    if (opt_key == "extruders_count")
        wxGetApp().sidebar().set_extruders_count(boost::any_cast<size_t>(value));
*/
    if (m_postpone_update_ui) {
        // It means that not all values are rolled to the system/last saved values jet.
        // And call of the update() can causes a redundant check of the config values,
        // see https://github.com/prusa3d/PrusaSlicer/issues/7146
        return;
    }

    update();
}

void AbstractEditor::activate_option(const std::string& opt_key, const wxString& category)
{
    wxString page_title = translate_category(category, m_type);

    // We should to activate a tab with searched option, if it doesn't.
    // And do it before finding of the cur_item to avoid a case when Tab isn't activated jet and all treeItems are invisible
//!    wxGetApp().mainframe->select_tab(this);

    auto cur_item = m_treectrl->GetFirstVisibleItem();
    if (!cur_item)
        return;

    while (cur_item) {
        auto title = m_treectrl->GetItemText(cur_item);
        if (page_title != title) {
            cur_item = m_treectrl->GetNextVisible(cur_item);
            continue;
        }

        m_treectrl->SelectItem(cur_item);
        break;
    }

    auto set_focus = [](wxWindow* win) {
        win->SetFocus();
#ifdef WIN32
        if (wxTextCtrl* text = dynamic_cast<wxTextCtrl*>(win))
            text->SetSelection(-1, -1);
        else if (wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>(win))
            spin->SetSelection(-1, -1);
#endif // WIN32
    };

    Field* field = get_field(opt_key);

    // focused selected field
    if (field)
        set_focus(field->getWindow());
    else if (category == "Single extruder MM setup") {
        // When we show and hide "Single extruder MM setup" page, 
        // related options are still in the search list
        // So, let's hightlighte a "single_extruder_multi_material" option, 
        // as a "way" to show hidden page again
        field = get_field("single_extruder_multi_material");
        if (field)
            set_focus(field->getWindow());
    }

    m_highlighter.init(get_custom_ctrl_with_blinking_ptr(opt_key));
}
//!
void AbstractEditor::cache_config_diff(const std::vector<std::string>& selected_options, const DynamicPrintConfig* config/* = nullptr*/)
{
    m_cache_config.apply_only(config ? *config : m_state->edited_preset.config, selected_options);
}
//!
void AbstractEditor::apply_config_from_cache()
{
    bool was_applied = false;
    // check and apply extruders count for printer preset
    if (m_type == Slic3r::Preset::TYPE_PRINTER)
        was_applied = static_cast<EditorPrinter*>(this)->apply_extruder_cnt_from_cache();

    if (!m_cache_config.empty()) {
        m_state->edited_preset.config.apply(m_cache_config);
        m_cache_config.clear();

        was_applied = true;
    }

    if (was_applied)
        update_dirty();
}

void AbstractEditor::build_preset_description_line(ConfigOptionsGroup* optgroup)
{
    auto description_line = [this](wxWindow* parent) {
        return description_line_widget(parent, &m_parent_preset_description_line);
    };

    Line line = Line{ "", "" };
    line.full_width = 1;

    line.append_widget(description_line);

    optgroup->append_line(line);
}

void AbstractEditor::update_preset_description_line()
{
    const Slic3r::Preset* parent = m_state->selected_preset_parent;
    const Slic3r::Preset& preset = m_state->edited_preset;

    wxString description_line;

    if (preset.is_default) {
        description_line = _L("This is a default preset.");
    } else if (preset.is_system) {
        description_line = _L("This is a system preset.");
    } else if (parent == nullptr) {
        description_line = _L("Current preset is inherited from the default preset.");
    } else {
        std::string name = parent->name;
        boost::replace_all(name, "&", "&&");
        description_line = _L("Current preset is inherited from") + ":\n\t" + from_u8(name);
    }

    if (preset.is_default || preset.is_system)
        description_line += "\n\t" + _L("It can't be deleted or modified.") +
                            "\n\t" + _L("Any modifications should be saved as a new preset inherited from this one.") +
                            "\n\t" + _L("To do that please specify a new name for the preset.");

    if (parent && parent->vendor)
    {
        description_line += "\n\n" + _L("Additional information:") + "\n";
        description_line += "\t" + _L("vendor") + ": " + (m_type == Slic3r::Preset::TYPE_PRINTER ? "\n\t\t" : "") + parent->vendor->name +
                            ", ver: " + parent->vendor->config_version.to_string();
        if (m_type == Slic3r::Preset::TYPE_PRINTER) {
            const std::string &printer_model = preset.config.opt_string("printer_model");
            if (! printer_model.empty())
                description_line += "\n\n\t" + _L("printer model") + ": \n\t\t" + printer_model;
            switch (preset.printer_technology()) {
            case ptFFF:
            {
                //FIXME add prefered_sla_material_profile for SLA
                const std::string              &default_print_profile = preset.config.opt_string("default_print_profile");
                const std::vector<std::string> &default_filament_profiles = preset.config.option<ConfigOptionStrings>("default_filament_profile")->values;
                if (!default_print_profile.empty())
                    description_line += "\n\n\t" + _L("default print profile") + ": \n\t\t" + default_print_profile;
                if (!default_filament_profiles.empty())
                {
                    description_line += "\n\n\t" + _L("default filament profile") + ": \n\t\t";
                    for (const std::string& profile : default_filament_profiles) {
                        if (&profile != &*default_filament_profiles.begin())
                            description_line += ", ";
                        description_line += from_u8(profile);
                    }
                }
                break;
            }
            case ptSLA:
            {
                //FIXME add prefered_sla_material_profile for SLA
                const std::string &default_sla_material_profile = preset.config.opt_string("default_sla_material_profile");
                if (!default_sla_material_profile.empty())
                    description_line += "\n\n\t" + _L("default SLA material profile") + ": \n\t\t" + default_sla_material_profile;

                const std::string &default_sla_print_profile = preset.config.opt_string("default_sla_print_profile");
                if (!default_sla_print_profile.empty())
                    description_line += "\n\n\t" + _L("default SLA print profile") + ": \n\t\t" + default_sla_print_profile;
                break;
            }
            default: break;
            }
        }
        else if (!preset.alias.empty())
        {
            description_line += "\n\n\t" + _L("full profile name")     + ": \n\t\t" + preset.name;
            description_line += "\n\t"   + _L("symbolic profile name") + ": \n\t\t" + preset.alias;
        }
    }

    m_parent_preset_description_line->SetText(description_line, false);

    Layout();
}

//!
bool AbstractEditor::validate_custom_gcode(const wxString& title, const std::string& gcode)
{
    std::vector<std::string> tags;
    bool invalid = GCodeProcessor::contains_reserved_tags(gcode, 5, tags);
    if (invalid) {
        std::string lines = ":\n";
        for (const std::string& keyword : tags)
            lines += ";" + keyword + "\n";

        wxString reports = WX::format_wxstr(
            _L_PLURAL("The following line %s contains reserved keywords.\nPlease remove it, as it may cause problems in G-code visualization and printing time estimation.", 
                      "The following lines %s contain reserved keywords.\nPlease remove them, as they may cause problems in G-code visualization and printing time estimation.", 
                      tags.size()),
            lines);
        WX::MessageDialog dialog(/*wxGetApp().mainframe*/nullptr, reports, _L("Found reserved keywords in") + " " + _(title), wxICON_WARNING | wxOK); //!
        dialog.ShowModal();

    }
    return !invalid;
}

void AbstractEditor::edit_custom_gcode(const t_config_option_key& opt_key)
{
    EditGCodeDialog dlg = EditGCodeDialog(this, opt_key, get_custom_gcode(opt_key), m_ccc);
    if (dlg.ShowModal() == wxID_OK) {
        set_custom_gcode(opt_key, dlg.get_edited_gcode());
        update_dirty();
        update();
    }
}

const std::string& AbstractEditor::get_custom_gcode(const t_config_option_key& opt_key)
{
    return m_config->opt_string(opt_key);
}

void AbstractEditor::set_custom_gcode(const t_config_option_key& opt_key, const std::string& value)
{
    DynamicPrintConfig new_conf = *m_config;
    new_conf.set_key_value(opt_key, new ConfigOptionString(value));
    load_config(new_conf);
}

wxSizer* AbstractEditor::description_line_widget(wxWindow* parent, ogStaticText* *StaticText, wxString text /*= wxEmptyString*/)
{
    *StaticText = new ogStaticText(parent, text);
    (*StaticText)->SetFont(WX::w_config()->normal_font());

    auto sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(*StaticText, 1, wxEXPAND|wxALL, 0);
    return sizer;
}

// Legend for OptionsGroups                                     column's   name         tooltip
void AbstractEditor::create_legend(PageShp page, const std::vector<std::pair<std::string, std::string>>& columns, ConfigOptionMode mode, bool is_wider /*= false*/)
{
    auto optgroup = page->new_optgroup("");
    auto line = Line{ "", "" };

    ConfigOptionDef def;
    def.type = coString;
    def.width = is_wider ? Field::def_width_wider() : Field::def_width();
    def.gui_type = ConfigOptionDef::GUIType::legend;
    def.mode = mode;

    for (auto& [name, tooltip] : columns) {
        def.tooltip = tooltip;
        def.set_default_value(new ConfigOptionString{ into_u8(_(name)) });

        auto option = Option(def, name + "_legend");
        line.append_option(option);
    }

    optgroup->append_line(line);
}

std::optional<ConfigOptionsGroupShp> AbstractEditor::get_option_group(const Page* page, const std::string& title)
{
    auto og_it = std::find_if(
        page->optgroups.begin(), page->optgroups.end(),
        [&](const ConfigOptionsGroupShp& og) {
            return og->title == title;
        }
    );
    if (og_it == page->optgroups.end())
        return {};
    return *og_it;
}

void AbstractEditor::add_options_into_line(ConfigOptionsGroupShp &optgroup,
                                  const std::vector<SamePair<std::string>> &prefixes,
                                  const std::string &optkey,
                                  const std::string &preprefix/* = std::string()*/)
{
    auto opt = optgroup->get_option(preprefix + prefixes.front().first + optkey);
    Line line{ opt.opt.label, "" };
    line.full_width = 1;
    for (auto &prefix : prefixes) {
        opt = optgroup->get_option(preprefix + prefix.first + optkey);
        opt.opt.label = prefix.second;
        opt.opt.width = 12; // TODO
        line.append_option(opt);
    }
    optgroup->append_line(line);
}

void AbstractEditor::validate_custom_gcode_cb(const wxString& title, const t_config_option_key& opt_key, const boost::any& value)
{
    this->m_validate_custom_gcodes_was_shown = !AbstractEditor::validate_custom_gcode(title, boost::any_cast<std::string>(value));
    this->update_dirty();
    this->on_value_change(opt_key, value);  //? Why on_value_change() instead of just update()
}

bool AbstractEditor::is_prusa_printer() const
{
    std::string printer_model = m_ccc->printer.edited_preset.config.opt_string("printer_model");
    return printer_model == "SL1" || printer_model == "SL1S" || printer_model == "M1";
}

void AbstractEditor::update_ui_items_related_on_parent_preset(const Slic3r::Preset* selected_preset_parent)
{
    m_is_default_preset = selected_preset_parent != nullptr && selected_preset_parent->is_default;

    m_bmp_non_system = selected_preset_parent ? &m_bmp_value_unlock : &m_bmp_white_bullet;
    m_ttg_non_system = selected_preset_parent ? &m_ttg_value_unlock : &m_ttg_white_bullet_ns;
    m_tt_non_system  = selected_preset_parent ? &m_tt_value_unlock  : &m_ttg_white_bullet_ns;
}

// Initialize the UI from the current preset
void AbstractEditor::load_current_preset()
{
    const Slic3r::Preset& preset = m_state->edited_preset;

    update();
    if (m_type == Slic3r::Preset::TYPE_PRINTER) {
        // For the printer profile, generate the extruder pages.
        if (preset.printer_technology() == ptFFF)
            on_preset_loaded();
        /*  //!
        else
            wxGetApp().sidebar().update_objects_list_extruder_column(1);
        // Check and show "Physical printer" page if needed
        wxGetApp().show_printer_webview_tab();
*/
    }
    // Reload preset pages with the new configuration values.
    reload_config();

    update_ui_items_related_on_parent_preset(m_state->selected_preset_parent);

    {
        // checking out if this Tab exists till this moment
//!        if (!wxGetApp().checked_tab(this))
//!            return;

//!        on_presets_changed();
        if (m_type == Slic3r::Preset::TYPE_PRINTER) {
            if (m_state->edited_preset.printer_technology() == ptFFF) {
                static_cast<EditorPrinter*>(this)->set_init_extruders_cnt(static_cast<const ConfigOptionFloats*>(m_state->selected_preset->config.option("nozzle_diameter"))->values.size());
                const Slic3r::Preset* parent_preset = m_state->selected_preset_parent;
                static_cast<EditorPrinter*>(this)->set_sys_extruders_cnt(parent_preset == nullptr ? 0 :
                    static_cast<const ConfigOptionFloats*>(parent_preset->config.option("nozzle_diameter"))->values.size());
            }
        }

        m_opt_status_value = (m_state->selected_preset_parent ? osSystemValue : 0) | osInitValue;
        init_options_list();
        update_visibility();
        update_sla_prusa_specific_visibility();
        update_changed_ui();
    }
}

//Regerenerate content of the page tree.
void AbstractEditor::rebuild_page_tree()
{
    // get label of the currently selected item
    const auto sel_item = m_treectrl->GetSelection();
    const auto selected = sel_item ? m_treectrl->GetItemText(sel_item) : "";
    const auto rootItem = m_treectrl->GetRootItem();

    wxTreeItemId item;

    // Delete/Append events invoke wxEVT_TREE_SEL_CHANGED event.
    // To avoid redundant clear/activate functions call
    // suppress activate page before page_tree rebuilding
    m_disable_tree_sel_changed_event = true;
    m_treectrl->DeleteChildren(rootItem);

    for (auto p : m_pages)
    {
        if (!p->get_show())
            continue;
        auto itemId = m_treectrl->AppendItem(rootItem, translate_category(p->title(), m_type), p->iconID());
        m_treectrl->SetItemTextColour(itemId, p->get_item_colour());
        m_treectrl->SetItemFont(itemId, WX::w_config()->normal_font());
        if (translate_category(p->title(), m_type) == selected)
            item = itemId;
    }
    if (!item) {
        // this is triggered on first load, so we don't disable the sel change event
        item = m_treectrl->GetFirstVisibleItem();
    }

    // allow activate page before selection of a page_tree item
    m_disable_tree_sel_changed_event = false;
    if (item)
        m_treectrl->SelectItem(item);
}

void AbstractEditor::update_preset_choice()
{
//!    m_presets_choice->update();
}

void AbstractEditor::clear_pages()
{
    // invalidated highlighter, if any exists
    m_highlighter.invalidate();
    m_page_sizer->Clear(true);
    // clear pages from the controlls
    for (auto p : m_pages)
        p->clear();

    // nulling pointers
    m_parent_preset_description_line = nullptr;

    m_compatible_printers.checkbox  = nullptr;
    m_compatible_printers.btn       = nullptr;

    m_compatible_prints.checkbox    = nullptr;
    m_compatible_prints.btn         = nullptr;
}

void AbstractEditor::update_description_lines()
{
    if (m_active_page && m_active_page->title() == "Dependencies" && m_parent_preset_description_line)
        update_preset_description_line();
}

void AbstractEditor::activate_selected_page(std::function<void()> throw_if_canceled)
{
    if (!m_active_page)
        return;

    m_active_page->activate(m_mode, throw_if_canceled);

    if (m_active_page->title() == "Dependencies") {
        if (m_compatible_printers.checkbox)
            this->compatible_widget_reload(m_compatible_printers);
        if (m_compatible_prints.checkbox)
            this->compatible_widget_reload(m_compatible_prints);
    }

    update_sla_prusa_specific_visibility();
    update_changed_ui();
    update_description_lines();
    toggle_options();
}

#ifdef WIN32
// Override the wxCheckForInterrupt to process inperruptions just from key or mouse
// and to avoid an unwanted early call of CallAfter()
static bool CheckForInterrupt(wxWindow* wnd)
{
    wxCHECK(wnd, false);

    MSG msg;
    while (::PeekMessage(&msg, ((HWND)((wnd)->GetHWND())), WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    while (::PeekMessage(&msg, ((HWND)((wnd)->GetHWND())), WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return true;
}
#endif //WIN32

bool AbstractEditor::tree_sel_change_delayed()
{
    // There is a bug related to Ubuntu overlay scrollbars, see https://github.com/prusa3d/PrusaSlicer/issues/898 and https://github.com/prusa3d/PrusaSlicer/issues/952.
    // The issue apparently manifests when Show()ing a window with overlay scrollbars while the UI is frozen. For this reason,
    // we will Thaw the UI prematurely on Linux. This means destroing the no_updates object prematurely.
#ifdef __linux__
    std::unique_ptr<wxWindowUpdateLocker> no_updates(new wxWindowUpdateLocker(this));
#else
    /* On Windows we use DoubleBuffering during rendering,
     * so on Window is no needed to call a Freeze/Thaw functions.
     * But under OSX (builds compiled with MacOSX10.14.sdk) wxStaticBitmap rendering is broken without Freeze/Thaw call.
     */
//#ifdef __WXOSX__  // Use Freeze/Thaw to avoid flickering during clear/activate new page
    wxWindowUpdateLocker noUpdates(this);
//#endif
#endif

    Page* page = nullptr;
    const auto sel_item = m_treectrl->GetSelection();
    const auto selection = sel_item ? m_treectrl->GetItemText(sel_item) : "";
    for (auto p : m_pages)
        if (translate_category(p->title(), m_type) == selection)
        {
            page = p.get();
            m_is_nonsys_values = page->is_nonsys_values;
            m_is_modified_values = page->is_modified_values;
            break;
        }
    if (page == nullptr || m_active_page == page)
        return false;

    // clear pages from the controls
    m_active_page = page;
    
    auto throw_if_canceled = std::function<void()>([this](){
#ifdef WIN32
            CheckForInterrupt(m_treectrl);
            if (m_page_switch_planned)
                throw UIBuildCanceled();
#else // WIN32
            (void)this; // silence warning
#endif
        });

    try {
        clear_pages();
        throw_if_canceled();
  
//!        if (wxGetApp().mainframe!=nullptr && wxGetApp().mainframe->is_active_and_shown_tab(this))
            activate_selected_page(throw_if_canceled);

        #ifdef __linux__
            no_updates.reset(nullptr);
        #endif

        update_undo_buttons();
        throw_if_canceled();

        m_hsizer->Layout();
        throw_if_canceled();
        Refresh();
    } catch (const UIBuildCanceled&) {
	    if (m_active_page)
		    m_active_page->clear();
        return true;
    }

    return false;
}

void AbstractEditor::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_TAB)
        m_treectrl->Navigate(event.ShiftDown() ? wxNavigationKeyEvent::IsBackward : wxNavigationKeyEvent::IsForward);
    else
        event.Skip();
}

void AbstractEditor::create_line_with_widget(ConfigOptionsGroup* optgroup, const std::string& opt_key, const std::string& path, widget_t widget)
{
    Line line = optgroup->create_single_option_line(opt_key);
    line.widget = widget;
    line.label_path = path;

    // set default undo ui
    line.set_undo_bitmap(&m_bmp_white_bullet);
    line.set_undo_to_sys_bitmap(&m_bmp_white_bullet);
    line.set_undo_tooltip(&m_tt_white_bullet);
    line.set_undo_to_sys_tooltip(&m_tt_white_bullet);
    line.set_label_colour(&m_default_text_clr);

    optgroup->append_line(line);
}

// Return a callback to create a Tab widget to mark the preferences as compatible / incompatible to the current printer.
wxSizer* AbstractEditor::compatible_widget_create(wxWindow* parent, PresetDependencies &deps)
{
    deps.checkbox = CheckBox::GetNewWin(parent, _L("All"));
    deps.checkbox->SetFont(WX::w_config()->normal_font());
    WX::w_config()->UpdateDarkUI(deps.checkbox);
    deps.btn = new WX::ScalableButton(parent, wxID_ANY, "printer", WX::format_wxstr(" %s %s", _L("Set"), dots),
                                  wxDefaultSize, wxDefaultPosition, wxBU_LEFT | wxBU_EXACTFIT);
    deps.btn->SetFont(WX::w_config()->normal_font());
    deps.btn->SetSize(deps.btn->GetBestSize());

    auto sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add((deps.checkbox), 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add((deps.btn), 0, wxALIGN_CENTER_VERTICAL);

    deps.checkbox->Bind(wxEVT_CHECKBOX, ([this, &deps](wxCommandEvent e)
    {
        const bool is_checked = CheckBox::GetValue(deps.checkbox);
        deps.btn->Enable(!is_checked);
        // All printers have been made compatible with this preset.
        if (is_checked)
            this->load_key_value(deps.key_list, std::vector<std::string> {});
        this->get_field(deps.key_condition)->toggle(is_checked);
        this->update_changed_ui();
    }) );
/*  //! use callback on mainframe to get print/printer lists from presetbundle
    deps.btn->Bind(wxEVT_BUTTON, ([this, parent, &deps](wxCommandEvent e)
    {
        // Collect names of non-default non-external profiles.
        PrinterTechnology printer_technology = m_preset_bundle->printers.get_edited_preset().printer_technology();
        PresetCollection &depending_presets  = (deps.type == Slic3r::Preset::TYPE_PRINTER) ? m_preset_bundle->printers :
                (printer_technology == ptFFF) ? m_preset_bundle->prints : m_preset_bundle->sla_prints;
        wxArrayString presets;
        for (size_t idx = 0; idx < depending_presets.size(); ++ idx)
        {
            Slic3r::Preset& preset = depending_presets.preset(idx);
            bool add = ! preset.is_default && ! preset.is_external;
            if (add && deps.type == Slic3r::Preset::TYPE_PRINTER)
                // Only add printers with the same technology as the active printer.
                add &= preset.printer_technology() == printer_technology;
            if (add)
                presets.Add(from_u8(preset.name));
        }

        wxMultiChoiceDialog dlg(parent, deps.dialog_title, deps.dialog_label, presets);
        WX::w_config()->UpdateDlgDarkUI(&dlg);
        // Collect and set indices of depending_presets marked as compatible.
        wxArrayInt selections;
        auto *compatible_printers = dynamic_cast<const ConfigOptionStrings*>(m_config->option(deps.key_list));
        if (compatible_printers != nullptr || !compatible_printers->values.empty())
            for (auto preset_name : compatible_printers->values)
                for (size_t idx = 0; idx < presets.GetCount(); ++idx)
                    if (presets[idx] == preset_name) {
                        selections.Add(idx);
                        break;
                    }
        dlg.SetSelections(selections);
        std::vector<std::string> value;
        // Show the dialog.
        if (dlg.ShowModal() == wxID_OK) {
            selections.Clear();
            selections = dlg.GetSelections();
            for (auto idx : selections)
                value.push_back(presets[idx].ToUTF8().data());
            if (value.empty()) {
                CheckBox::SetValue(deps.checkbox, true);
                deps.btn->Disable();
            }
            // All depending_presets have been made compatible with this preset.
            this->load_key_value(deps.key_list, value);
            this->update_changed_ui();
        }
    }));
*/
    return sizer;
}

bool AbstractEditor::validate_custom_gcodes()
{
    if (m_type != Slic3r::Preset::TYPE_FILAMENT &&
        (m_type != Slic3r::Preset::TYPE_PRINTER || static_cast<EditorPrinter*>(this)->printer_technology != ptFFF))
        return true;
    if (m_active_page->title() != L("Custom G-code"))
        return true;

    // When we switch Settings tab after editing of the custom g-code, then warning message could ba already shown after KillFocus event
    // and then it's no need to show it again
    if (m_validate_custom_gcodes_was_shown) {
        m_validate_custom_gcodes_was_shown = false;
        return true;
    }

    bool valid = true;
    for (auto opt_group : m_active_page->optgroups) {
        assert(opt_group->opt_map().size() == 1);
        if (!opt_group->is_activated())
            break;
        std::string key = opt_group->opt_map().begin()->first;
        if (key == "autoemit_temperature_commands")
            continue;
        valid &= validate_custom_gcode(opt_group->title, boost::any_cast<std::string>(opt_group->get_value(key)));
        if (!valid)
            break;
    }
    return valid;
}

void AbstractEditor::compatible_widget_reload(PresetDependencies &deps)
{
    Field* field = this->get_field(deps.key_condition);
    if (!field)
        return;

    bool has_any = ! m_config->option<ConfigOptionStrings>(deps.key_list)->values.empty();
    has_any ? deps.btn->Enable() : deps.btn->Disable();
    CheckBox::SetValue(deps.checkbox, !has_any);

    field->toggle(! has_any);
}

void AbstractEditor::fill_icon_descriptions()
{
    m_icon_descriptions.emplace_back(&m_bmp_value_lock, L("LOCKED LOCK"),
        // TRN Description for "LOCKED LOCK"
        L("indicates that the settings are the same as the system (or default) values for the current option group"));

    m_icon_descriptions.emplace_back(&m_bmp_value_unlock, L("UNLOCKED LOCK"),
        // TRN Description for "UNLOCKED LOCK"
        L("indicates that some settings were changed and are not equal to the system (or default) values for "
        "the current option group.\n"
        "Click the UNLOCKED LOCK icon to reset all settings for current option group to "
        "the system (or default) values."));

    m_icon_descriptions.emplace_back(&m_bmp_white_bullet, L("WHITE BULLET"),
        // TRN Description for "WHITE BULLET"
        L("for the left button: indicates a non-system (or non-default) preset,\n"
          "for the right button: indicates that the settings hasn't been modified."));

    m_icon_descriptions.emplace_back(&m_bmp_value_revert, L("BACK ARROW"),
        // TRN Description for "BACK ARROW"
        L("indicates that the settings were changed and are not equal to the last saved preset for "
        "the current option group.\n"
        "Click the BACK ARROW icon to reset all settings for the current option group to "
        "the last saved preset."));

    m_icon_descriptions.emplace_back(&m_bmp_edit_value, L("EDIT VALUE"),
        // TRN Description for "EDIT VALUE" in the Help dialog (the icon is currently used only to edit custom gcodes).
        L("clicking this icon opens a dialog allowing to edit this value."));
}

void AbstractEditor::set_tooltips_text()
{
    // --- Tooltip text for reset buttons (for whole options group)
    // Text to be shown on the "Revert to system" aka "Lock to system" button next to each input field.
    m_ttg_value_lock =		_L("LOCKED LOCK icon indicates that the settings are the same as the system (or default) values "
                                "for the current option group");
    m_ttg_value_unlock =	_L("UNLOCKED LOCK icon indicates that some settings were changed and are not equal "
                                "to the system (or default) values for the current option group.\n"
                                "Click to reset all settings for current option group to the system (or default) values.");
    m_ttg_white_bullet_ns =	_L("WHITE BULLET icon indicates a non system (or non default) preset.");
    m_ttg_non_system =		&m_ttg_white_bullet_ns;
    // Text to be shown on the "Undo user changes" button next to each input field.
    m_ttg_white_bullet =	_L("WHITE BULLET icon indicates that the settings are the same as in the last saved "
                                "preset for the current option group.");
    m_ttg_value_revert =	_L("BACK ARROW icon indicates that the settings were changed and are not equal to "
                                "the last saved preset for the current option group.\n"
                                "Click to reset all settings for the current option group to the last saved preset.");

    // --- Tooltip text for reset buttons (for each option in group)
    // Text to be shown on the "Revert to system" aka "Lock to system" button next to each input field.
    m_tt_value_lock =		_L("LOCKED LOCK icon indicates that the value is the same as the system (or default) value.");
    m_tt_value_unlock =		_L("UNLOCKED LOCK icon indicates that the value was changed and is not equal "
                                "to the system (or default) value.\n"
                                "Click to reset current value to the system (or default) value.");
    // 	m_tt_white_bullet_ns=	_L("WHITE BULLET icon indicates a non system preset."));
    m_tt_non_system =		&m_ttg_white_bullet_ns;
    // Text to be shown on the "Undo user changes" button next to each input field.
    m_tt_white_bullet =		_L("WHITE BULLET icon indicates that the value is the same as in the last saved preset.");
    m_tt_value_revert =		_L("BACK ARROW icon indicates that the value was changed and is not equal to the last saved preset.\n"
                                "Click to reset current value to the last saved preset.");
}

WX::ConfigManipulation AbstractEditor::get_config_manipulation()
{
    auto load_config = [this]()
    {
        update_dirty();
        // Initialize UI components with the config values.
        reload_config();
        update();
    };

    auto cb_toggle_field = [this](const t_config_option_key& opt_key, bool toggle, int opt_index) {
        return toggle_option(opt_key, toggle, opt_index);
    };

    auto cb_value_change = [this](const std::string& opt_key, const boost::any& value) {
        return on_value_change(opt_key, value);
    };

    auto config_manipulation = WX::ConfigManipulation(load_config, cb_toggle_field, cb_value_change);
    config_manipulation.set_message_dialog_parent(this);

    return config_manipulation;
}


} 
