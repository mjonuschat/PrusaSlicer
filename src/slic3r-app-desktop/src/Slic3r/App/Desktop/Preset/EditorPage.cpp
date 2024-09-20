///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, Lukáš Matěna @lukasmatena, Lukáš Hejl @hejllukas, Vojtěch Bubník @bubnikv, Pavel Mikuš @Godrak, Tomáš Mészáros @tamasmeszaros, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Vojtěch Král @vojtechkral
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "EditorPage.hpp"
#include "AbstractEditor.hpp"

#include <wx/window.h>
#include <wx/sizer.h>

namespace Slic3r::App::Desktop::Preset {

using namespace Config;

Page::Page(wxWindow* parent, const wxString& title, int iconID) :
        m_parent(parent),
        m_title(title),
        m_iconID(iconID)
{
    m_vsizer = (wxBoxSizer*)parent->GetSizer();
    m_item_color = &WX::w_config()->get_label_clr_default();
}

void Page::reload_config()
{
    for (auto group : optgroups)
        group->reload_config();
}

void Page::update_visibility(ConfigOptionMode mode, bool update_contolls_visibility)
{
    bool ret_val = false;
    for (auto group : optgroups) {
        ret_val = (update_contolls_visibility     ? 
                   group->update_visibility(mode) :  // update visibility for all controlls in group
                   group->is_visible(mode)           // just detect visibility for the group
                   ) || ret_val;
    }

    m_show = ret_val;
}

void Page::activate(ConfigOptionMode mode, std::function<void()> throw_if_canceled)
{
    for (auto group : optgroups) {
        if (!group->activate(throw_if_canceled))
            continue;
        m_vsizer->Add(group->sizer, 0, wxEXPAND | (group->is_legend_line() ? (wxLEFT|wxTOP) : wxALL), 10);
        group->update_visibility(mode);
        group->reload_config();
        throw_if_canceled();
    }
}

void Page::clear()
{
    for (auto group : optgroups)
        group->clear();
}

void Page::msw_rescale()
{
    for (auto group : optgroups)
        group->msw_rescale();
}

void Page::sys_color_changed()
{
    for (auto group : optgroups)
        group->sys_color_changed();
}

void Page::refresh()
{
    for (auto group : optgroups)
        group->refresh();
}

Field* Page::get_field(const t_config_option_key& opt_key, int opt_index /*= -1*/) const
{
    Field* field = nullptr;
    for (auto opt : optgroups) {
        field = opt->get_fieldc(opt_key, opt_index);
        if (field != nullptr)
            return field;
    }
    return field;
}

Line* Page::get_line(const t_config_option_key& opt_key)
{
    for (auto opt : optgroups)
        if (Line* line = opt->get_line(opt_key))
            return line;
    return nullptr;
}

bool Page::set_value(const t_config_option_key& opt_key, const boost::any& value) {
    bool changed = false;
    for(auto optgroup: optgroups) {
        if (optgroup->set_value(opt_key, value))
            changed = true ;
    }
    return changed;
}

ConfigOptionsGroupShp Page::new_optgroup(const wxString& title, int noncommon_label_width /*= -1*/)
{
    //! config_ have to be "right"
    ConfigOptionsGroupShp optgroup = std::make_shared<ConfigOptionsGroup>(m_parent, title, m_config_interactor, true);
    if (noncommon_label_width >= 0)
        optgroup->label_width = noncommon_label_width;

#ifdef __WXOSX__
    AbstractEditor* editor = static_cast<AbstractEditor*>(parent()->GetParent()->GetParent());
#else
    AbstractEditor* editor = static_cast<AbstractEditor*>(parent()->GetParent());
#endif
    optgroup->set_config_category_and_type(m_title, static_cast<AbstractEditor*>(editor)->type());
    optgroup->on_change = [editor](t_config_option_key opt_key, boost::any value) {
        // This function will be called from OptionGroup.
        editor->update_dirty();
        editor->on_value_change(opt_key, value);
    };

    optgroup->get_initial_config = [editor]() {
        return editor->config_interactor().preset_state().edited_preset.config;
    };

    optgroup->get_sys_config = [editor]() {
        return editor->config_interactor().preset_state().selected_preset_parent->config;
    };

    optgroup->have_sys_config = [editor]() {
        return editor->config_interactor().preset_state().selected_preset_parent != nullptr;
    };

    optgroup->rescale_extra_column_item = [](wxWindow* win) {
        auto *ctrl = dynamic_cast<wxStaticBitmap*>(win);
        if (ctrl == nullptr)
            return;

        ctrl->SetBitmap(reinterpret_cast<WX::ScalableBitmap*>(ctrl->GetClientData())->bmp());
    };

    optgroups.push_back(optgroup);

    return optgroup;
}

const ConfigOptionsGroupShp Page::get_optgroup(const wxString& title) const
{
    for (ConfigOptionsGroupShp optgroup : optgroups) {
        if (optgroup->title == title)
            return optgroup;
    }

    return nullptr;
}


} 
