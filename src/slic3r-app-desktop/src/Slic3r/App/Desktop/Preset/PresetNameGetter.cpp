///|/ Copyright (c) Prusa Research 2020 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Bubník @bubnikv
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "PresetNameGetter.hpp"

#include "Slic3r/App/WX/WidgetsConfig.hpp"
#include "Slic3r/App/WX/wxExtensions.hpp"
#include "Slic3r/App/WX/BitmapGetters.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/format.hpp"

#include "Slic3r/App/WX/Widgets/ComboBox.hpp"

#include <cstddef>
#include <vector>
#include <string>

#include <wx/window.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/statbmp.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#ifdef _WIN32
#include <wx/msw/private.h>
#endif

#include "libslic3r/PresetBundle.hpp"

//!#include "I18N.hpp"
#define L(s) s
static wxString             _(const wxString& s)                            { return s; };
static wxString            _L(const wxString& s)                            { return s; };
static std::string       _u8L(const std::string& s)                         { return s; };

namespace Slic3r::App::Desktop::Preset {

constexpr auto BORDER_W = 10;

//-----------------------------------------------
//          PresetNameGetter
//-----------------------------------------------

std::string PresetNameGetter::get_init_preset_name(const Slic3r::Preset* selected_preset, const std::string &suffix)
{
    std::string preset_name = selected_preset->is_default ? "Untitled" :
                              selected_preset->is_system ? WX::format("%1% - %2%", selected_preset->name, suffix) :
                              selected_preset->name;

    // if name contains extension
    if (boost::iends_with(preset_name, ".ini")) {
        size_t len = preset_name.length() - 4;
        preset_name.resize(len);
    }

    return preset_name;
}

void PresetNameGetter::init_casei_preset_names()
{
    m_casei_preset_names.clear();

    auto add_names_from_collection = [this](const PresetCollection& presets) {
        for (const Slic3r::Preset& preset : presets)
            if (!preset.is_default)
                m_casei_preset_names.emplace_back(PresetName({ boost::to_lower_copy<std::string>(preset.name), preset.name }));
        };

    if (m_presets) {
        // This item is a part of SavePresetDialog and will check names inside selected PresetCollection
        m_casei_preset_names.reserve(m_presets->size());
        add_names_from_collection(*m_presets);
    }
    else // This item is a part of ConfigWizard and will check names inside all PresetCollections in respect to the m_printer_technology
    if (m_preset_bundle) {
        auto types_list = PresetBundle::types_list(m_printer_technology);

        size_t presets_cnt = 0;
        for (const Slic3r::Preset::Type& type : types_list)
            presets_cnt += m_preset_bundle->get_presets(type).size();
        m_casei_preset_names.reserve(presets_cnt);

        for (const Slic3r::Preset::Type& type : types_list)
            add_names_from_collection(m_preset_bundle->get_presets(type));
    }

    std::sort(m_casei_preset_names.begin(), m_casei_preset_names.end());
}

void PresetNameGetter::init_input_name_ctrl(wxBoxSizer *input_name_sizer, const std::string preset_name)
{
    if (m_use_text_ctrl) {
#ifdef _WIN32
        long style = wxBORDER_SIMPLE;
#else
        long style = 0L;
#endif
        m_text_ctrl = new wxTextCtrl(m_parent, wxID_ANY, WX::from_u8(preset_name), wxDefaultPosition, wxSize(35 * WX::w_config()->em_unit(), -1), style);
        WX::w_config()->UpdateDarkUI(m_text_ctrl);
        m_text_ctrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { update(); });

        input_name_sizer->Add(m_text_ctrl,1, wxEXPAND, BORDER_W);
    }
    else {
        std::vector<std::string> values;
        for (const Slic3r::Preset& preset : *m_presets) {
            if (preset.is_default || preset.is_system || preset.is_external)
                continue;
            values.push_back(preset.name);
        }

        m_combo = new WX::Widgets::ComboBox(m_parent, wxID_ANY, "", wxDefaultPosition, wxSize(35 * WX::w_config()->em_unit(), -1), 0, nullptr, wxTE_PROCESS_ENTER | DD_NO_CHECK_ICON);
        for (const std::string&value : values)
            m_combo->Append(WX::from_u8(value));
        m_combo->SetValue(WX::from_u8(preset_name));

        m_combo->GetTextCtrl()->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { 
            update(); });
#ifdef __WXOSX__
        // Under OSX wxEVT_TEXT wasn't invoked after change selection in combobox,
        // So process wxEVT_COMBOBOX too
        m_combo->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent&) { update(); });
#endif //__WXOSX__

        input_name_sizer->Add(m_combo,    1, wxEXPAND, BORDER_W);
    }
}

static std::map<Slic3r::Preset::Type, std::string> TOP_LABELS =
{
    // type                             Save settings    
    { Slic3r::Preset::Type::TYPE_PRINT,         L("Save print settings as")   },
    { Slic3r::Preset::Type::TYPE_SLA_PRINT,     L("Save print settings as")   },
    { Slic3r::Preset::Type::TYPE_FILAMENT,      L("Save filament settings as")},
    { Slic3r::Preset::Type::TYPE_SLA_MATERIAL,  L("Save material settings as")},
    { Slic3r::Preset::Type::TYPE_PRINTER,       L("Save printer settings as") },
};

PresetNameGetter::PresetNameGetter(wxWindow* parent, wxBoxSizer* sizer, const Slic3r::Preset* selected_preset, PresetCollection* presets, 
                                   const std::string& suffix, bool as_text_ctrl, bool show_label):
    m_type(selected_preset->type),
    m_use_text_ctrl(as_text_ctrl),
    m_parent(parent),
    m_presets(presets),
    m_valid_bmp(new wxStaticBitmap(m_parent, wxID_ANY, *WX::get_bmp_bundle("tick_mark"))),
    m_valid_label(new wxStaticText(m_parent, wxID_ANY, ""))
{
    build(sizer, get_init_preset_name(selected_preset, suffix), show_label);
}

PresetNameGetter::PresetNameGetter(wxWindow* parent, wxBoxSizer* sizer, const std::string& def_name, PresetBundle* preset_bundle, PrinterTechnology pt /*= ptFFF*/):
    m_preset_name(def_name),
    m_preset_bundle(preset_bundle),
    m_printer_technology(pt),
    m_parent(parent),
    m_valid_bmp(new wxStaticBitmap(m_parent, wxID_ANY, *WX::get_bmp_bundle("tick_mark"))),
    m_valid_label(new wxStaticText(m_parent, wxID_ANY, ""))
{
    build(sizer, m_preset_name);
    update();
}

void PresetNameGetter::build(wxBoxSizer* sizer, std::string preset_name, bool show_label /*= false*/)
{
    m_valid_label->SetFont(WX::w_config()->bold_font());

    wxBoxSizer* input_name_sizer = new wxBoxSizer(wxHORIZONTAL);
    input_name_sizer->Add(m_valid_bmp,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, BORDER_W);
    init_input_name_ctrl(input_name_sizer, preset_name);

    init_casei_preset_names();

    if (show_label)
        sizer->Add(new wxStaticText(m_parent, wxID_ANY, _(TOP_LABELS.at(m_type)) + ":"),   0, wxEXPAND | wxTOP| wxBOTTOM, BORDER_W);

    sizer->Add(input_name_sizer,0, wxEXPAND | (show_label ? 0 : wxTOP) | wxBOTTOM, BORDER_W);
    sizer->Add(m_valid_label,   0, wxEXPAND | wxLEFT,   3*BORDER_W);

}

std::string PresetNameGetter::get_conflict_name(const std::string& preset_name) const
{
    if (!m_casei_preset_names.empty()) {
        const std::string lower_name = boost::to_lower_copy<std::string>(preset_name);
        auto it = Slic3r::lower_bound_by_predicate(m_casei_preset_names.begin(), m_casei_preset_names.end(),
                                                   [lower_name](const auto& l) { return l.casei_name < lower_name;  });
        if (it != m_casei_preset_names.end() && it->casei_name == lower_name)
            return it->name;
    }
    return std::string();
}

std::string PresetNameGetter::preset_name() const
{
    if (m_use_text_ctrl)
        return m_preset_name;

    const std::string existed_preset_name = get_conflict_name(m_preset_name);
    if (existed_preset_name.empty())
        return m_preset_name;

    return existed_preset_name;
}

const Slic3r::Preset* PresetNameGetter::get_existing_preset() const 
{
    std::string existed_preset_name = get_conflict_name(m_preset_name);
    if (existed_preset_name.empty()) {
        // Preset has not been not found in the sorted list of non-default presets. Try the defaults.
        return nullptr;
    }

    if (m_presets)
        return m_presets->find_preset(existed_preset_name, false);

    if (m_preset_bundle) {
        for (const Slic3r::Preset::Type& type : PresetBundle::types_list(m_printer_technology)) {
            const PresetCollection& presets = m_preset_bundle->get_presets(type);
            if (const Slic3r::Preset* preset = presets.find_preset(existed_preset_name, false))
                return preset;
        }
    }
    return nullptr;
}

void PresetNameGetter::update()
{
    m_preset_name = WX::into_u8(m_use_text_ctrl ? m_text_ctrl->GetValue() : m_combo->GetLabel());

    m_valid_type = ValidationType::Valid;
    wxString info_line;

    const char* unusable_symbols = "<>[]:/\\|?*\"";

    const std::string unusable_suffix = PresetCollection::get_suffix_modified();//"(modified)";
    for (size_t i = 0; i < std::strlen(unusable_symbols); i++) {
        if (m_preset_name.find_first_of(unusable_symbols[i]) != std::string::npos) {
            info_line = _L("The following characters are not allowed in the name") + ": " + unusable_symbols;
            m_valid_type = ValidationType::NoValid;
            break;
        }
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.find(unusable_suffix) != std::string::npos) {
        info_line = _L("The following suffix is not allowed in the name") + ":\n\t" +
                    WX::from_u8(unusable_suffix);
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name == "- default -") {
        info_line = _L("This name is reserved, use another.");
        m_valid_type = ValidationType::NoValid;
    }

    const Slic3r::Preset* existing = get_existing_preset();
    if (m_valid_type == ValidationType::Valid && existing && (existing->is_default || existing->is_system)) {
        info_line = m_use_text_ctrl ? _L("This name is used for a system profile name, use another.") :
                             _L("Cannot overwrite a system profile.");
        info_line += "\n" + WX::format_wxstr("(%1%)", existing->name);
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && existing && (existing->is_external)) {
        info_line = m_use_text_ctrl ? _L("This name is used for an external profile name, use another.") :
                             _L("Cannot overwrite an external profile.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && existing)
    {
        if (m_presets && m_preset_name == m_presets->get_selected_preset_name()) {
            if ((!m_use_text_ctrl && m_presets->get_edited_preset().is_dirty) ||
                m_preset_bundle) // means that we save modifications from the DiffDialog
                info_line = _L("Save preset modifications to existing user profile");
            m_valid_type = ValidationType::Valid;
        }
        else {
            if (existing->is_compatible)
                info_line = WX::format_wxstr(_u8L("Preset with name \"%1%\" already exists."), existing->name);
            else
                info_line = WX::format_wxstr(_u8L("Preset with name \"%1%\" already exists and is incompatible with selected printer."), existing->name);
            info_line += "\n" + (m_use_text_ctrl ? _L("Note: This preset will be replaced after renaming") :
                                                   _L("Note: Preset modifications will be saved exactly into this preset"));
            m_valid_type = ValidationType::Warning;
        }
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.empty()) {
        info_line = _L("The name cannot be empty.");
        m_valid_type = ValidationType::NoValid;
    }

#ifdef __WXMSW__
    const int max_path_length = MAX_PATH;
#else
    const int max_path_length = 255;
#endif

    if (m_valid_type == ValidationType::Valid && m_presets && m_presets->path_from_name(m_preset_name).length() >= max_path_length) {
        info_line = _L("The name is too long.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.find_first_of(' ') == 0) {
        info_line = _L("The name cannot start with space character.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_preset_name.find_last_of(' ') == m_preset_name.length()-1) {
        info_line = _L("The name cannot end with space character.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type == ValidationType::Valid && m_presets && m_presets->get_preset_name_by_alias(m_preset_name) != m_preset_name) {
        info_line = _L("The name cannot be the same as a preset alias name.");
        m_valid_type = ValidationType::NoValid;
    }

    if (m_valid_type != ValidationType::NoValid && m_cb_info_line_extention) {        
        if (wxString ext = m_cb_info_line_extention(); !ext.IsEmpty())
            info_line += "\n\n" + ext;
    }

    m_valid_label->SetLabel(info_line);
    m_valid_label->Show(!info_line.IsEmpty());

    update_valid_bmp();

    if (m_cb_update_extra_info_line)
        m_cb_update_extra_info_line(m_valid_type != ValidationType::NoValid, m_preset_name);

    m_parent->Layout();
    if (m_combo)
        m_combo->Refresh();
}

void PresetNameGetter::update_valid_bmp()
{
    std::string bmp_name =  m_valid_type == ValidationType::Warning ? "exclamation_manifold" :
                            m_valid_type == ValidationType::NoValid ? "exclamation"          : "tick_mark" ;
    m_valid_bmp->SetBitmap(*WX::get_bmp_bundle(bmp_name));
}

void PresetNameGetter::enable(bool enable /*= true*/)
{
    m_valid_label->Enable(enable);
    m_valid_bmp->Enable(enable);
    m_use_text_ctrl ? m_text_ctrl->Enable(enable) : m_combo->Enable(enable);
}

}
