///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, Lukáš Hejl @hejllukas, Vojtěch Bubník @bubnikv, Pavel Mikuš @Godrak, Tomáš Mészáros @tamasmeszaros, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Král @vojtechkral
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/


#include "SubstitutionManager.hpp"
#include "../Config/Field.hpp"
#include "Slic3r/App/WX/MsgDialog.hpp"
#include "Slic3r/App/WX/StringConversions.hpp"
#include "Slic3r/App/WX/WidgetsConfig.hpp"

#include <wx/window.h>
#include <wx/sizer.h>

//#include "I18N.hpp"
#define _L(s)   s

namespace Slic3r::App::Desktop::Preset {

using WX::from_u8;
using WX::into_u8;

using namespace Config;

// G-code substitutions

void SubstitutionManager::init(DynamicPrintConfig* config, wxWindow* parent, wxFlexGridSizer* grid_sizer)
{
    m_config = config;
    m_parent = parent;
    m_grid_sizer = grid_sizer;
    m_em = WX::w_config()->em_unit(parent);

    m_substitutions = m_config->option<ConfigOptionStrings>("gcode_substitutions")->values;
    m_chb_match_single_lines.clear();
}

void SubstitutionManager::validate_length()
{
    if ((m_substitutions.size() % 4) != 0) {
        WX::WarningDialog(m_parent, _L("Value of gcode_substitutions parameter will be cut to valid length"),
                                    _L("Invalid length of gcode_substitutions parameter")).ShowModal();
        m_substitutions.resize(m_substitutions.size() - (m_substitutions.size() % 4));
        // save changes from m_substitutions to config 
        m_config->option<ConfigOptionStrings>("gcode_substitutions")->values = m_substitutions;
    }
}

bool SubstitutionManager::is_compatible_with_ui()
{
    if (int(m_substitutions.size() / 4) != m_grid_sizer->GetEffectiveRowsCount() - 1) {
        WX::ErrorDialog(m_parent, _L("Invalid compatibility between UI and BE"), false).ShowModal();
        return false;
    }
    return true;
};

bool SubstitutionManager::is_valid_id(int substitution_id, const wxString& message)
{
    if (int(m_substitutions.size() / 4) < substitution_id) {
        WX::ErrorDialog(m_parent, message, false).ShowModal();
        return false;
    }
    return true;
}

void SubstitutionManager::create_legend()
{
    if (!m_grid_sizer->IsEmpty())
        return;
    // name of the first column is empty
    m_grid_sizer->Add(new wxStaticText(m_parent, wxID_ANY, wxEmptyString));

    // Legend for another columns
    auto legend_sizer = new wxBoxSizer(wxHORIZONTAL); // "Find", "Replace", "Notes"
    legend_sizer->Add(new wxStaticText(m_parent, wxID_ANY, _L("Find")),         3, wxEXPAND);
    legend_sizer->Add(new wxStaticText(m_parent, wxID_ANY, _L("Replace with")), 3, wxEXPAND);
    legend_sizer->Add(new wxStaticText(m_parent, wxID_ANY, _L("Notes")),      2, wxEXPAND);

    m_grid_sizer->Add(legend_sizer, 1, wxEXPAND);
}

// delete substitution_id from substitutions
void SubstitutionManager::delete_substitution(int substitution_id)
{
    validate_length();
    if (!is_valid_id(substitution_id, _L("Invalid substitution_id to delete")))
        return;

    // delete substitution
    std::vector<std::string>& substitutions = m_config->option<ConfigOptionStrings>("gcode_substitutions")->values;
    substitutions.erase(std::next(substitutions.begin(), substitution_id * 4), std::next(substitutions.begin(), substitution_id * 4 + 4));

    call_ui_update();

    // update grid_sizer
    update_from_config();
}

// Add substitution line
void SubstitutionManager::add_substitution( int substitution_id, 
                                            const std::string& plain_pattern, 
                                            const std::string& format, 
                                            const std::string& params,
                                            const std::string& notes)
{
    bool call_after_layout = false;
    
    if (substitution_id < 0) {
        if (m_grid_sizer->IsEmpty()) {
            create_legend();
        }
        substitution_id = m_grid_sizer->GetEffectiveRowsCount() - 1;

        // create new substitution
        // it have to be added to config too
        for (size_t i = 0; i < 4; i ++)
            m_substitutions.push_back(std::string());

        // save changes from config to m_substitutions
        m_config->option<ConfigOptionStrings>("gcode_substitutions")->values = m_substitutions;

        call_after_layout = true;
    }

    auto del_btn = new WX::ScalableButton(m_parent, wxID_ANY, "cross");
    del_btn->Bind(wxEVT_BUTTON, [substitution_id, this](wxEvent&) {
        delete_substitution(substitution_id);
    });

    m_grid_sizer->Add(del_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, int(0.5*m_em));

    auto top_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto add_text_editor = [substitution_id, top_sizer, this](const wxString& value, int opt_pos, int proportion) {
        auto editor = new WX::Widgets::TextInput(m_parent, value, "", "", wxDefaultPosition, wxSize(15 * m_em, wxDefaultCoord), wxTE_PROCESS_ENTER);

        editor->SetFont(WX::w_config()->normal_font());
        WX::w_config()->UpdateDarkUI(editor);
        top_sizer->Add(editor, proportion, wxALIGN_CENTER_VERTICAL | wxRIGHT, m_em);

        editor->Bind(wxEVT_TEXT_ENTER, [this, editor, substitution_id, opt_pos](wxEvent& e) {
#if !defined(__WXGTK__)
            e.Skip();
#endif // __WXGTK__
            edit_substitution(substitution_id, opt_pos, into_u8(editor->GetValue()));
        });

        editor->Bind(wxEVT_KILL_FOCUS, [this, editor, substitution_id, opt_pos](wxEvent& e) {
            e.Skip();
            edit_substitution(substitution_id, opt_pos, into_u8(editor->GetValue()));
        });
    };

    add_text_editor(from_u8(plain_pattern), 0, 3);
    add_text_editor(from_u8(format),        1, 3);
    add_text_editor(from_u8(notes),         3, 2);

    auto params_sizer = new wxBoxSizer(wxHORIZONTAL);
    bool regexp              = strchr(params.c_str(), 'r') != nullptr || strchr(params.c_str(), 'R') != nullptr;
    bool case_insensitive    = strchr(params.c_str(), 'i') != nullptr || strchr(params.c_str(), 'I') != nullptr;
    bool whole_word          = strchr(params.c_str(), 'w') != nullptr || strchr(params.c_str(), 'W') != nullptr;
    bool match_single_line   = strchr(params.c_str(), 's') != nullptr || strchr(params.c_str(), 'S') != nullptr;

    auto chb_regexp = CheckBox::GetNewWin(m_parent, _L("Regular expression"));
    CheckBox::SetValue(chb_regexp, regexp);
    params_sizer->Add(chb_regexp, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, m_em);

    auto chb_case_insensitive = CheckBox::GetNewWin(m_parent, _L("Case insensitive"));
    CheckBox::SetValue(chb_case_insensitive, case_insensitive);
    params_sizer->Add(chb_case_insensitive, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, m_em);

    auto chb_whole_word = CheckBox::GetNewWin(m_parent, _L("Whole word"));
    CheckBox::SetValue(chb_whole_word, whole_word);
    params_sizer->Add(chb_whole_word, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, m_em);

    auto chb_match_single_line = CheckBox::GetNewWin(m_parent, _L("Match single line"));
    CheckBox::SetValue(chb_match_single_line, match_single_line);
    chb_match_single_line->Show(regexp);
    m_chb_match_single_lines.emplace_back(chb_match_single_line);

    params_sizer->Add(chb_match_single_line, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, m_em);

    for (wxWindow* chb : std::initializer_list<wxWindow*>{ chb_regexp, chb_case_insensitive, chb_whole_word, chb_match_single_line }) {
        chb->SetFont(WX::w_config()->normal_font());
        chb->Bind(wxEVT_CHECKBOX, [this, substitution_id, chb_regexp, chb_case_insensitive, chb_whole_word, chb_match_single_line](wxCommandEvent e) {
            std::string value = std::string();
            if (CheckBox::GetValue(chb_regexp))
                value += "r";
            if (CheckBox::GetValue(chb_case_insensitive))
                value += "i";
            if (CheckBox::GetValue(chb_whole_word))
                value += "w";
            if (CheckBox::GetValue(chb_match_single_line))
                value += "s";

            chb_match_single_line->Show(CheckBox::GetValue(chb_regexp));
            m_grid_sizer->Layout();

            edit_substitution(substitution_id, 2, value);
        });
    }

    auto v_sizer = new wxBoxSizer(wxVERTICAL);
    v_sizer->Add(top_sizer, 1, wxEXPAND);
    v_sizer->Add(params_sizer, 1, wxEXPAND|wxTOP|wxBOTTOM, int(0.5* m_em));
    m_grid_sizer->Add(v_sizer, 1, wxEXPAND);

    if (call_after_layout) {
        m_parent->GetParent()->Layout();
        call_ui_update();
    }
}

void SubstitutionManager::update_from_config()
{
    std::vector<std::string>& subst = m_config->option<ConfigOptionStrings>("gcode_substitutions")->values;
    if (m_substitutions == subst && !subst.empty()  && m_grid_sizer->IsShown(1)) {
        // just update visibility for chb_match_single_lines
        int subst_id = 0;
        assert(m_chb_match_single_lines.size() == size_t(subst.size()/4));
        for (size_t i = 0; i < subst.size(); i += 4) {
            const std::string& params = subst[i + 2];
            const bool         regexp = strchr(params.c_str(), 'r') != nullptr || strchr(params.c_str(), 'R') != nullptr;
            m_chb_match_single_lines[subst_id++]->Show(regexp);
        }

        // "gcode_substitutions" values didn't changed in config. There is no need to update/recreate controls
        return;
    }

    m_substitutions = subst;

    if (!m_grid_sizer->IsEmpty()) {
        m_grid_sizer->Clear(true);
        m_chb_match_single_lines.clear();
    }

    if (subst.empty())
        hide_delete_all_btn();
    else
        create_legend();

    validate_length();

    int subst_id = 0;
    for (size_t i = 0; i < subst.size(); i += 4)
        add_substitution(subst_id++, subst[i], subst[i + 1], subst[i + 2], subst[i + 3]);

    m_parent->GetParent()->Layout();
}

void SubstitutionManager::delete_all()
{
    m_substitutions.clear();
    m_config->option<ConfigOptionStrings>("gcode_substitutions")->values.clear();
    call_ui_update();

    if (!m_grid_sizer->IsEmpty()) {
        m_grid_sizer->Clear(true);
        m_chb_match_single_lines.clear();
    }

    m_parent->GetParent()->Layout();
}

void SubstitutionManager::edit_substitution(int substitution_id, int opt_pos, const std::string& value)
{
    validate_length();
    if(!is_compatible_with_ui() || !is_valid_id(substitution_id, _L("Invalid substitution_id to edit")))
        return;

    m_substitutions[substitution_id * 4 + opt_pos] = value;
    // save changes from m_substitutions to config 
    m_config->option<ConfigOptionStrings>("gcode_substitutions")->values = m_substitutions;

    call_ui_update();
}

bool SubstitutionManager::is_empty_substitutions()
{
    return m_config->option<ConfigOptionStrings>("gcode_substitutions")->values.empty();
}

}
