///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "PresetNameGetter.hpp"
#include "SavePresetDialog.hpp"

#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/wxExtensions.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/I18N.hpp"
#include "Slic3r/App/WX/format.hpp"

#include <cstddef>
#include <vector>
#include <string>

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/statbmp.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>

#include "libslic3r/PresetBundle.hpp" // IWYU pragma: keep

namespace Slic3r::App::Desktop::Preset {

using WX::_L;

constexpr auto BORDER_W = 10;


//-----------------------------------------------
//          SavePresetDialog
//-----------------------------------------------

SavePresetDialog::SavePresetDialog( wxWindow* parent, 
                                    std::vector<const Slic3r::Preset*> selected_presets, 
                                    const PresetBundle* preset_bundle,
                                    std::string suffix /*= ""*/, 
                                    bool template_filament/* =false*/,
                                    std::string ph_printer_name/* = ""*/)
    : wxDialog(parent, wxID_ANY, selected_presets.size() == 1 ? _L("Save preset") : _L("Save presets"),
                wxDefaultPosition, wxSize(45 * WX::w_config()->em_unit(), 5 * WX::w_config()->em_unit()), wxDEFAULT_DIALOG_STYLE | wxICON_WARNING),
    m_preset_bundle(preset_bundle),
    m_ph_printer_name(ph_printer_name)
{
    build(selected_presets, suffix, template_filament);

#ifndef __WXOSX__
    this->Bind(wxEVT_DPI_CHANGED, [this](wxDPIChangedEvent& event)
    {
        event.Skip();
        this->dpi_changed();
    });
#endif

}

SavePresetDialog::SavePresetDialog(wxWindow* parent, const Slic3r::Preset* selected_preset, const PresetBundle* preset_bundle)
    : wxDialog(parent, wxID_ANY, _L("Rename preset"), wxDefaultPosition, wxSize(45 * WX::w_config()->em_unit(), 5 * WX::w_config()->em_unit()), wxDEFAULT_DIALOG_STYLE | wxICON_WARNING),
    m_use_for_rename(true),
    m_preset_bundle(preset_bundle)
{
    build(std::vector<const Slic3r::Preset*>{selected_preset});
}

SavePresetDialog::~SavePresetDialog()
{
    for (auto  item : m_items) {
        delete item;
    }
}

void SavePresetDialog::build(std::vector<const Slic3r::Preset*> selected_presets, std::string suffix, bool template_filament)
{
    this->SetFont(WX::w_config()->normal_font());

#ifndef __WXMSW__
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif 

    if (suffix.empty())
        // TRN Suffix for the preset name. Have to be a noun.
        suffix = _CTX_utf8(L_CONTEXT("Copy", "PresetName"), "PresetName");

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    m_presets_sizer = new wxBoxSizer(wxVERTICAL);

    const bool is_for_multiple_save = selected_presets.size() > 1;
    for (const Slic3r::Preset* sel_preset : selected_presets)
        AddItem(sel_preset, suffix, is_for_multiple_save);

    // Add dialog's buttons
    wxStdDialogButtonSizer* btns = this->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    wxButton* btnOK = static_cast<wxButton*>(this->FindWindowById(wxID_OK, this));
    btnOK->Bind(wxEVT_BUTTON,    [this](wxCommandEvent&)        { accept(); });
    btnOK->Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& evt)   { evt.Enable(enable_ok_btn()); });

    topSizer->Add(m_presets_sizer,  0, wxEXPAND | wxALL, BORDER_W);
    
    // Add checkbox for Template filament saving
    if (template_filament && selected_presets.size() == 1 && selected_presets[0]->type == Slic3r::Preset::TYPE_FILAMENT) {
        m_template_filament_checkbox = new wxCheckBox(this, wxID_ANY, _L("Save as profile derived from current printer only."));
        wxBoxSizer* check_sizer = new wxBoxSizer(wxVERTICAL);
        check_sizer->Add(m_template_filament_checkbox);
        topSizer->Add(check_sizer, 0, wxEXPAND | wxALL, BORDER_W);
    }

    topSizer->Add(btns,             0, wxEXPAND | wxALL, BORDER_W);

    SetSizer(topSizer);
    topSizer->SetSizeHints(this);

    this->CenterOnScreen();

#ifdef _WIN32
    WX::w_config()->UpdateDlgDarkUI(this);
#endif
}

void SavePresetDialog::AddItem(const Slic3r::Preset* selected_preset, const std::string& suffix, bool is_for_multiple_save)
{
    auto type = selected_preset->type;
    PresetNameGetter* item = new PresetNameGetter{ this, m_presets_sizer, selected_preset, &m_preset_bundle->get_presets(type), suffix, m_use_for_rename, is_for_multiple_save };

    if (type == Slic3r::Preset::TYPE_PRINTER) {
        m_old_preset_name = selected_preset->name;
        add_info_for_edit_ph_printer(m_presets_sizer);

        item->set_cb_info_line_extention([this]() { return m_info_line_extention; });

        item->set_cb_update_extra_info_line([this](bool is_valid_name, const std::string& preset_name) {
            this->update_info_for_edit_ph_printer(is_valid_name, preset_name);
        });
    }

    item->update();

    m_items.emplace_back(item);
}

std::string SavePresetDialog::get_name()
{
    return m_items.front()->preset_name();
}

std::string SavePresetDialog::get_name(Slic3r::Preset::Type type)
{
    for (const PresetNameGetter* item : m_items)
        if (item->type() == type)
            return item->preset_name();
    return "";
}

bool SavePresetDialog::get_template_filament_checkbox()
{
    if (m_template_filament_checkbox)
    {
        return m_template_filament_checkbox->GetValue();
    }
    return false;
}

void SavePresetDialog::set_info_line_extentions(const wxString& info_line_extention)
{
    m_info_line_extention = info_line_extention;
    for (auto* item : m_items)
        item->update();
}

bool SavePresetDialog::enable_ok_btn() const
{
    for (const PresetNameGetter* item : m_items)
        if (!item->is_valid())
            return false;

    return true;
}

void SavePresetDialog::add_info_for_edit_ph_printer(wxBoxSizer* sizer)
{
    wxString msg_text = WX::format_wxstr(_u8L("You have selected physical printer \"%1%\" \n"
                                              "with related printer preset \"%2%\""),
                                               m_ph_printer_name, m_old_preset_name);
    m_label = new wxStaticText(this, wxID_ANY, msg_text);
    m_label->SetFont(WX::w_config()->bold_font());

    m_action = ChangePreset;
    m_radio_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticBox* action_stb = new wxStaticBox(this, wxID_ANY, "");
    if (!wxOSX) action_stb->SetBackgroundStyle(wxBG_STYLE_PAINT);
    action_stb->SetFont(WX::w_config()->bold_font());

    wxStaticBoxSizer* stb_sizer = new wxStaticBoxSizer(action_stb, wxVERTICAL);
    for (int id = 0; id < 3; id++) {
        wxRadioButton* btn = new wxRadioButton(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, id == 0 ? wxRB_GROUP : 0);
        btn->SetValue(id == int(ChangePreset));
        btn->Bind(wxEVT_RADIOBUTTON, [this, id](wxCommandEvent&) { m_action = (ActionType)id; });
        stb_sizer->Add(btn, 0, wxEXPAND | wxTOP, 5);
    }
    m_radio_sizer->Add(stb_sizer, 1, wxEXPAND | wxTOP, 2*BORDER_W);

    sizer->Add(m_label,         0, wxEXPAND | wxLEFT | wxTOP,   3*BORDER_W);
    sizer->Add(m_radio_sizer,   1, wxEXPAND | wxLEFT,           3*BORDER_W);
}

void SavePresetDialog::update_info_for_edit_ph_printer(bool is_valid_name, const std::string& preset_name)
{
    bool show = is_valid_name && !m_ph_printer_name.empty() && m_old_preset_name != preset_name;

    m_label->Show(show);
    m_radio_sizer->ShowItems(show);
    if (!show) {
        this->SetMinSize(wxSize(100,50));
        return;
    }

    if (wxSizerItem* sizer_item = m_radio_sizer->GetItem(size_t(0))) {
        if (wxStaticBoxSizer* stb_sizer = static_cast<wxStaticBoxSizer*>(sizer_item->GetSizer())) {
            wxString msg_text = WX::format_wxstr(_L("What would you like to do with \"%1%\" preset after saving?"), preset_name);
            stb_sizer->GetStaticBox()->SetLabel(msg_text);

            wxString choices[] = { WX::format_wxstr(_L("Change \"%1%\" to \"%2%\" for this physical printer \"%3%\""), m_old_preset_name, preset_name, m_ph_printer_name),
                                   WX::format_wxstr(_L("Add \"%1%\" as a next preset for the the physical printer \"%2%\""), preset_name, m_ph_printer_name),
                                   WX::format_wxstr(_L("Just switch to \"%1%\" preset"), preset_name) };

            size_t n = 0;
            for (const wxString& label : choices)
                stb_sizer->GetItem(n++)->GetWindow()->SetLabel(label);
        }
        Refresh();
    }
}

bool SavePresetDialog::Layout()
{
    const bool ret = wxDialog::Layout();
    this->Fit();
    return ret;
}

void SavePresetDialog::dpi_changed()
{
    const int& em = WX::w_config()->em_unit();

    WX::msw_buttons_rescale(this, em, { wxID_OK, wxID_CANCEL });

    for (PresetNameGetter* item : m_items)
        item->update_valid_bmp();

    SetMinSize(wxSize(100, 50));

    Fit();
    Refresh();
}

void SavePresetDialog::update_physical_printers(const std::string& preset_name)
{
    if (m_action == UndefAction || m_ph_printer_name.empty())
        return;

/*    //! move into PresetInteractor
    PhysicalPrinterCollection& physical_printers = m_preset_bundle->physical_printers;
    std::string printer_preset_name = physical_printers.get_selected_printer_preset_name();

    if (m_action == Switch)
        // unselect physical printer, if it was selected
        physical_printers.unselect_printer();
    else
    {
        PhysicalPrinter printer = physical_printers.get_selected_printer();

        if (m_action == ChangePreset)
            printer.delete_preset(printer_preset_name);

        if (printer.add_preset(preset_name))
            physical_printers.save_printer(printer);

        physical_printers.select_printer(printer.get_full_name(preset_name));
    } 
*/
}

void SavePresetDialog::accept()
{
    for (PresetNameGetter* item : m_items) {
        if (item->type() == Slic3r::Preset::TYPE_PRINTER) {
            update_physical_printers(item->preset_name());
            break;
        }
    }

    EndModal(wxID_OK);
}

}
