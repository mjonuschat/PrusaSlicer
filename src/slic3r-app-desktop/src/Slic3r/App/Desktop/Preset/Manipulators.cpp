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


#include "Manipulators.hpp"
#include "EditorPresetComboBox.hpp"
#include "SavePresetDialog.hpp"
//!#include "PhysicalPrinterDialog.hpp"

#include "Slic3r/App/WX/Scalable.hpp"
#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/BitmapGetters.hpp"
#include "Slic3r/App/WX/MsgDialog.hpp"
#include "Slic3r/App/WX/format.hpp"

#include "Slic3r/Biz/Preset/PresetState.hpp"

#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"

#include <boost/exception/diagnostic_information.hpp>

#include <wx/window.h>
#include <wx/button.h>
#include <wx/string.h>
#include <wx/bmpbuttn.h>

//!#include "I18N.hpp"
#define _u8L(s) s
#define L(s) s
#define _(s) s
static wxString _L(const wxString& s) { return s; };
static wxString _L_PLURAL(const wxString& s1, const wxString& s2, int n) { return s1; };

namespace Slic3r::App::Desktop::Preset {

Manipulators::Manipulators(wxWindow* parent, EditorPresetComboBox* presets_list, Biz::Preset::PresetInteractor* preset_interactor) :
    wxBoxSizer(wxHORIZONTAL),
    m_parent(parent),
    m_presets_list(presets_list),
    m_preset_interactor(preset_interactor)
{
    //TRN Settings Tab: tooltip for toolbar button
    m_btn_save_preset = add_button("save", _L("Save preset"), [this]() { save_preset(); }, nullptr, 0);

    //TRN Settings Tab: tooltip for toolbar button
    m_btn_rename_preset = add_button("edit", _L("Rename preset"), [this]() { rename_preset(); }, [this]() { return m_can_rename_presets; });

    //TRN Settings Tab: tooltip for toolbar button
    m_btn_delete_preset = add_button("cross", _L("Delete preset"), [this]() { delete_preset(); }, [this]() { return m_can_delete_presets; });

    //TRN Settings Tab: tooltip for toolbar button
    m_detach_preset_btn = add_button("lock_open_sys", _L("Detach from system preset"), [this]() { detach_preset(); }, [this]() { return m_can_detach_presets; }, 20);

    if (presets_list->type() == Slic3r::Preset::Type::TYPE_PRINTER) {
        //TRN Settings Tab: tooltip for toolbar button
        m_btn_edit_ph_printer = add_button("cog", _L("Add physical printer"), 
            [this]() {
                if (m_presets_list->is_selected_physical_printer())
                    edit_physical_printer();
                else
                    add_physical_printer();
            }
        );
    }
    else
        m_btn_hide_incompatible_presets = add_button("flag_green", "", [this]() { toggle_show_hide_incompatible(); }, [this]() { return m_show_btn_incompatible_presets; });

    //TRN Settings Tab: tooltip for toolbar button
    m_btn_compare_preset = add_button("compare", _L("Compare preset with another"), [this]() { compare_preset(); }, nullptr, 50);

    m_parent->Refresh();
}

void Manipulators::update(const Biz::Preset::PresetState* state, const std::string& printer_model, const std::string& ph_printer_name)
{
    m_preset_state      = state;
    m_printer_model     = printer_model;
    m_ph_printer_name   = ph_printer_name;

    auto preset = m_preset_state->edited_preset;
    auto parent = m_preset_state->selected_preset_parent;

    const bool is_printer_and_selected_physical = m_presets_list->type() == Slic3r::Preset::TYPE_PRINTER && !m_ph_printer_name.empty();

    m_can_detach_presets = parent && parent->is_system && !preset.is_default;
    m_can_delete_presets = is_printer_and_selected_physical || (!preset.is_default && !preset.is_system);
    m_can_rename_presets = !is_printer_and_selected_physical && !preset.is_default && !preset.is_system && !preset.is_external;

    m_parent->Refresh();
}

void Manipulators::show_btn_incompatible_presets(bool show /*= true*/)
{
    m_show_btn_incompatible_presets = show;
    if (m_btn_hide_incompatible_presets && m_show_btn_incompatible_presets)
        update_compatibility_ui();
}

WX::ScalableButton* Manipulators::add_button(const std::string&      icon_name,
                                             const wxString&         tooltip,
                                             std::function<void()>   fn_on_click/* = nullptr*/,
                                             std::function<bool()>   fn_ui_update/* = nullptr*/,
                                             int                     left_space/* = 10*/)
{
    WX::ScalableButton *btn = new WX::ScalableButton(m_parent, wxID_ANY, icon_name);
    btn->SetToolTip(tooltip);
    this->Add(btn, 0, wxLEFT, left_space);

    if (fn_on_click)
        btn->Bind(wxEVT_BUTTON, [fn_on_click](wxCommandEvent&) { fn_on_click(); });

    if (fn_ui_update)
        btn->Bind(wxEVT_UPDATE_UI, [fn_ui_update](wxUpdateUIEvent& evt) { evt.Show(fn_ui_update()); });

    m_buttons.push_back(btn);

    return btn;
}

void Manipulators::sys_color_changed()
{
    // update buttons and cached bitmaps
    for (const auto btn : m_buttons)
        btn->sys_color_changed();
}

void Manipulators::edit_physical_printer()
{/*
    if (!m_preset_bundle->physical_printers.has_selection())
        return;

    PhysicalPrinterDialog dlg(this->GetParent(), this->GetString(this->GetSelection()));
    if (dlg.ShowModal() == wxID_OK) {
        update();
        wxGetApp().show_printer_webview_tab();
    }*/
}

void Manipulators::add_physical_printer()
{/*
    if (PhysicalPrinterDialog(this->GetParent(), wxEmptyString).ShowModal() == wxID_OK) {
        update();
        wxGetApp().show_printer_webview_tab();
    }*/
}

bool Manipulators::del_physical_printer(const wxString& note_string/* = wxEmptyString*/)
{
 /*   const std::string& printer_name = m_preset_bundle->physical_printers.get_selected_full_printer_name();
    if (printer_name.empty())
        return false;

    wxString msg;
    if (!note_string.IsEmpty())
        msg += note_string + "\n";
    msg += format_wxstr(_L("Are you sure you want to delete \"%1%\" printer?"), printer_name);

    if (WX::MessageDialog(this, msg, _L("Delete Physical Printer"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION).ShowModal() != wxID_YES)
        return false;

    m_preset_bundle->physical_printers.delete_selected_printer();

    this->update();

    if (dynamic_cast<PlaterPresetComboBox*>(this) != nullptr)
        wxGetApp().get_tab(m_type)->update_preset_choice();
    else if (dynamic_cast<EditorPresetComboBox*>(this) != nullptr)
    {
        wxGetApp().get_tab(m_type)->update_btns_enabling();
        wxGetApp().plater()->sidebar().update_presets(m_type);
    }*/
    return true;
}

void Manipulators::compare_preset()
{
//    wxGetApp().mainframe->diff_dialog.show(m_type);
}

void Manipulators::transfer_options(const std::string& name_from, const std::string& name_to, std::vector<std::string> options)
{
    if (options.empty())
        return;
/*
    Preset* preset_from = m_presets->find_preset(name_from);
    Preset* preset_to = m_presets->find_preset(name_to);

    if (m_type == Preset::TYPE_PRINTER) {
        auto it = std::find(options.begin(), options.end(), "extruders_count");
        if (it != options.end()) {
            // erase "extruders_count" option from the list
            options.erase(it);
            // cache the extruders count
            static_cast<TabPrinter*>(this)->cache_extruder_cnt(&preset_from->config);
        }
    }
    cache_config_diff(options, &preset_from->config);

    if (name_to != m_presets->get_edited_preset().name)
        select_preset(preset_to->name);

    apply_config_from_cache();
    load_current_preset();
    */
}

// Save the current preset into file.
// This removes the "dirty" flag of the preset, possibly creates a new preset under a new name,
// and activates the new preset.
// Wizard calls save_preset with a name "My Settings", otherwise no name is provided and this method
// opens a SavePresetDialog dialog.
void Manipulators::save_preset(std::string name /*= ""*/, bool detach)
{
    // since buttons(and choices too) don't get focus on Mac, we set focus manually
    // to the treectrl so that the EVT_* events are fired for the input field having
    // focus currently.is there anything better than this ?
//!	m_treectrl->OnSetFocus();

    Slic3r::Preset*      selected_preset = m_preset_state->selected_preset;
    Slic3r::Preset::Type type            = m_presets_list->type();
    bool from_template = false;
    std::string edited_printer;
    if (type == Slic3r::Preset::TYPE_FILAMENT && selected_preset->vendor && selected_preset->vendor->templates_profile) {
        edited_printer = m_printer_model;
        from_template = !edited_printer.empty();
    }

    if (name.empty()) {
        const std::string suffix = detach ? _u8L("Detached") : "";
        SavePresetDialog dlg(m_parent, { selected_preset }, m_presets_list->preset_bundle(), suffix, from_template, m_ph_printer_name);
        if (dlg.ShowModal() != wxID_OK)
            return;
        name = dlg.get_name();
        if (from_template)
            from_template = dlg.get_template_filament_checkbox();
    }
/*  //!
    if (detach && type == Slic3r::Preset::TYPE_PRINTER)
        m_config->opt_string("printer_model", true) = "";

    Slic3r::Preset&      edited_preset = m_preset_state->edited_preset;

    // Update compatible printers
    if (from_template && !edited_printer.empty()) {
        std::string cond = edited_preset.compatible_printers_condition();
        if (!cond.empty())
            cond += " and ";
        cond += "printer_model == \"" + edited_printer + "\"";
        edited_preset.config.opt_string("compatible_printers_condition") = cond;
    }

    // Save the preset into Slic3r::data_dir / presets / section_name / preset_name.ini
    save_current_preset(name, detach);

    if (detach && type == Preset::TYPE_PRINTER)
        wxGetApp().mainframe->on_config_changed(m_config);


    // Mark the print & filament enabled if they are compatible with the currently selected preset.
    // If saving the preset changes compatibility with other presets, keep the now incompatible dependent presets selected, however with a "red flag" icon showing that they are no more compatible.
    m_preset_bundle->update_compatible(PresetSelectCompatibleType::Never);
    // Add the new item into the UI component, remove dirty flags and activate the saved item.
    update_tab_ui();
    // Update the selection boxes at the plater.
    on_presets_changed();
    // If current profile is saved, "delete/rename preset" buttons have to be shown
    m_btn_delete_preset->Show();
    m_btn_rename_preset->Show(!m_presets_choice->is_selected_physical_printer());
    m_btn_delete_preset->GetParent()->Layout();

    if (type == Preset::TYPE_PRINTER)
        static_cast<TabPrinter*>(this)->m_initial_extruders_count = static_cast<TabPrinter*>(this)->m_extruders_count;

    // Parent preset is "default" after detaching, so we should to update UI values, related on parent preset  
    if (detach)
        update_ui_items_related_on_parent_preset(m_presets->get_selected_preset_parent());

    update_changed_ui();

    // If filament preset is saved for multi-material printer preset,
    // there are cases when filament comboboxs are updated for old (non-modified) colors,
    // but in full_config a filament_colors option aren't.
    if (type == Preset::TYPE_FILAMENT && wxGetApp().extruders_edited_cnt() > 1)
        wxGetApp().plater()->force_filament_colors_update();

    {
        // Profile compatiblity is updated first when the profile is saved.
        // Update profile selection combo boxes at the depending tabs to reflect modifications in profile compatibility.
        std::vector<Preset::Type> dependent;
        switch (type) {
        case Preset::TYPE_PRINT:
            dependent = { Preset::TYPE_FILAMENT };
            break;
        case Preset::TYPE_SLA_PRINT:
            dependent = { Preset::TYPE_SLA_MATERIAL };
            break;
        case Preset::TYPE_PRINTER:
            if (static_cast<const TabPrinter*>(this)->m_printer_technology == ptFFF)
                dependent = { Preset::TYPE_PRINT, Preset::TYPE_FILAMENT };
            else
                dependent = { Preset::TYPE_SLA_PRINT, Preset::TYPE_SLA_MATERIAL };
            break;
        default:
            break;
        }
        for (Preset::Type preset_type : dependent)
            wxGetApp().get_tab(preset_type)->update_tab_ui();
    }

    // update preset comboboxes in DiffPresetDlg
    wxGetApp().mainframe->diff_dialog.update_presets(type);

    if (detach)
        update_description_lines();
    */
}

void Manipulators::rename_preset()
{
    if (m_presets_list->is_selected_physical_printer())
        return;

    auto pb = m_presets_list->preset_bundle();
    SavePresetDialog dlg(m_parent, m_preset_state->selected_preset, pb);

    bool is_selected_ph_priter = false;

    if (m_presets_list->type() == Slic3r::Preset::TYPE_PRINTER) {
        if (pb->physical_printers.empty()) {
            // Check preset for rename in physical printers
            std::vector<std::string> ph_printers = pb->physical_printers.get_printers_with_preset(m_preset_state->selected_preset->name);
            if (!ph_printers.empty()) {
                wxString msg = _L_PLURAL("The physical printer below is based on the preset, you are going to rename.",
                                         "The physical printers below are based on the preset, you are going to rename.", ph_printers.size());
                for (const std::string& printer : ph_printers)
                    msg += "\n    \"" + WX::from_u8(printer) + "\",";
                msg.RemoveLast();
                msg += "\n" + _L_PLURAL("Note, that the selected preset will be renamed in this printer too.",
                    "Note, that the selected preset will be renamed in these printers too.", ph_printers.size()) + "\n\n";

                dlg.set_info_line_extentions(msg);
                is_selected_ph_priter = true;
            }
        }
    }

    // get new name

    if (dlg.ShowModal() != wxID_OK)
        return;

    const std::string new_name = dlg.get_name();
    if (new_name.empty() || new_name == m_preset_state->selected_preset->name)
        return;
/*  //! move into PresetInteractor
    const std::string old_name      = m_preset_state->selected_preset->name;
    const std::string old_file_name = m_preset_state->selected_preset->file;

    assert(old_name == m_preset_state->edited_preset.name);

    using namespace boost;
    try {
        // Note: selected preset can be changed, if in SavePresetDialog was selected name of existing preset
        Slic3r::Preset* selected_preset = m_preset_state->selected_preset;
        Slic3r::Preset& edited_preset   = m_preset_state->edited_preset;
        // rename selected and edited presets

        selected_preset->name = new_name;
        replace_last(selected_preset->file, old_name, new_name);

        edited_preset.name = new_name;
        replace_last(edited_preset.file, old_name, new_name);

        // rename file with renamed preset configuration

        filesystem::rename(old_file_name, selected_preset->file);

        // rename selected preset in printers, if it's needed

        if (is_selected_ph_priter)
            m_preset_bundle->physical_printers.rename_preset_in_printers(old_name, new_name);
    }
    catch (const exception& ex) {
        const std::string exception = diagnostic_information(ex);
        printf("Can't rename a preset : %s", exception.c_str());
    }

    // sort presets after renaming
    std::sort(m_presets->begin(), m_presets->end());
    // update selection
    select_preset_by_name(new_name, true);

    m_presets_choice->update();
    on_presets_changed();
*/
}

void Manipulators::detach_preset()
{
    bool system = m_preset_state->edited_preset.is_system;
    bool dirty = m_preset_state->edited_preset.is_dirty;
    wxString msg_text = system ?
        _L("A copy of the current system preset will be created, which will be detached from the system preset.") :
        _L("The current custom preset will be detached from the parent system preset.");

    if (dirty)
        msg_text += "\n\n" + _L("Modifications to the current profile will be saved.");
    msg_text += "\n\n" + _L("This action is not revertible.\nDo you want to proceed?");

    WX::MessageDialog dialog(m_parent, msg_text, _L("Detach preset"), wxICON_WARNING | wxYES_NO | wxCANCEL);
    if (dialog.ShowModal() == wxID_YES)
        save_preset(m_preset_state->edited_preset.is_system ? std::string() : m_preset_state->edited_preset.name, true);
}

// Called for a currently selected preset.
void Manipulators::delete_preset()
{
/*    auto current_preset = m_presets->get_selected_preset();
    // Don't let the user delete the ' - default - ' configuration.
    wxString action = current_preset.is_external ? _L("remove") : _L("delete");

    PhysicalPrinterCollection& physical_printers = m_preset_bundle->physical_printers;
    wxString msg;
    if (m_presets_choice->is_selected_physical_printer())
    {
        PhysicalPrinter& printer = physical_printers.get_selected_printer();
        if (printer.preset_names.size() == 1) {
            if (m_presets_choice->del_physical_printer(_L("It's a last preset for this physical printer."))) {
                // Hide "Physical printer" page
                wxGetApp().show_printer_webview_tab();
                Layout();
            }
            return;
        }

        msg = format_wxstr(_L("Are you sure you want to delete \"%1%\" preset from the physical printer \"%2%\"?"), current_preset.name, printer.name);
    }
    else
    {
        if (m_type == Preset::TYPE_PRINTER && !physical_printers.empty())
        {
            // Check preset for delete in physical printers
            // Ask a customer about next action, if there is a printer with just one preset and this preset is equal to delete
            std::vector<std::string> ph_printers = physical_printers.get_printers_with_preset(current_preset.name, false);
            std::vector<std::string> ph_printers_only = physical_printers.get_printers_with_only_preset(current_preset.name);

            if (!ph_printers.empty()) {
                msg += _L_PLURAL("The physical printer below is based on the preset, you are going to delete.",
                    "The physical printers below are based on the preset, you are going to delete.", ph_printers.size());
                for (const std::string& printer : ph_printers)
                    msg += "\n    \"" + from_u8(printer) + "\",";
                msg.RemoveLast();
                msg += "\n" + _L_PLURAL("Note, that the selected preset will be deleted from this printer too.",
                    "Note, that the selected preset will be deleted from these printers too.", ph_printers.size()) + "\n\n";
            }

            if (!ph_printers_only.empty()) {
                msg += _L_PLURAL("The physical printer below is based only on the preset, you are going to delete.",
                    "The physical printers below are based only on the preset, you are going to delete.", ph_printers_only.size());
                for (const std::string& printer : ph_printers_only)
                    msg += "\n    \"" + from_u8(printer) + "\",";
                msg.RemoveLast();
                msg += "\n" + _L_PLURAL("Note, that this printer will be deleted after deleting the selected preset.",
                    "Note, that these printers will be deleted after deleting the selected preset.", ph_printers_only.size()) + "\n\n";
            }
        }

        // TRN "remove/delete"
        msg += from_u8((boost::format(_u8L("Are you sure you want to %1% the selected preset?")) % action).str());
    }

    action = current_preset.is_external ? _L("Remove") : _L("Delete");
    // TRN Settings Tabs: Button in toolbar: "Remove/Delete"
    wxString title = format_wxstr(_L("%1% Preset"), action);
    if (current_preset.is_default ||
        //wxID_YES != wxMessageDialog(parent(), msg, title, wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION).ShowModal())
        wxID_YES != MessageDialog(parent(), msg, title, wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION).ShowModal())
        return;

    // if we just delete preset from the physical printer
    if (m_presets_choice->is_selected_physical_printer()) {
        PhysicalPrinter& printer = physical_printers.get_selected_printer();

        // just delete this preset from the current physical printer
        printer.delete_preset(m_presets->get_edited_preset().name);
        // select first from the possible presets for this printer
        physical_printers.select_printer(printer);

        this->select_preset(physical_printers.get_selected_printer_preset_name());
        return;
    }

    // delete selected preset from printers and printer, if it's needed
    if (m_type == Preset::TYPE_PRINTER && !physical_printers.empty())
        physical_printers.delete_preset_from_printers(current_preset.name);

    // Select will handle of the preset dependencies, of saving & closing the depending profiles, and
    // finally of deleting the preset.
    this->select_preset("", true);
    */
}

void Manipulators::toggle_show_hide_incompatible()
{
    m_show_incompatible_presets = !m_show_incompatible_presets;
    update_compatibility_ui();
}

void Manipulators::update_compatibility_ui()
{
    m_btn_hide_incompatible_presets->SetBitmap(*WX::get_bmp_bundle(m_show_incompatible_presets ? "flag_red" : "flag_green"));
    m_btn_hide_incompatible_presets->SetToolTip(m_show_incompatible_presets ?
        //TRN Settings Tab: tooltip for toolbar button
        _L("Both compatible an incompatible presets are shown. Click to hide presets not compatible with the current printer.") :
        //TRN Settings Tab: tooltip for toolbar button
        _L("Only compatible presets are shown. Click to show both the presets compatible and not compatible with the current printer."));

    m_presets_list->set_show_incompatible_presets(m_show_incompatible_presets);
    m_presets_list->update();
}

} 
