///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, Tomáš Mészáros @tamasmeszaros, Vojtěch Bubník @bubnikv, Lukáš Matěna @lukasmatena, Enrico Turri @enricoturri1966, Vojtěch Král @vojtechkral
///|/ 
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "../Config/OptionsGroup.hpp"
#include "libslic3r/Preset.hpp"
#include "Slic3r/Biz/Preset/IConfigInteractor.hpp"

#include <vector>
#include <memory>

#include <wx/string.h>
#include <wx/colour.h>

class wxWindows;
class wxBoxSizer;
class wxTreeCtrl;

namespace Slic3r::App::Desktop::Preset {

using namespace Config;

// Single Tab page containing a{ vsizer } of{ optgroups }
class Page
{
public:
    Page(wxWindow* parent, const wxString& title, int iconID);
    ~Page() {}

    void set_config_interactor(Biz::Preset::IConfigInteractor* config_interactor)
    { m_config_interactor = config_interactor; }
    void reload_config();
    void update_visibility(ConfigOptionMode mode, bool update_contolls_visibility);
    void activate(ConfigOptionMode mode, std::function<void()> throw_if_canceled);
    void clear();
    void msw_rescale();
    void sys_color_changed();
    void on_language_changed();
    void refresh();

    wxBoxSizer*                 vsizer() const { return m_vsizer; }
    wxWindow*                   parent() const { return m_parent; }
    const wxString&             title()  const { return m_title; }
    size_t                      iconID() const { return m_iconID; }

    Field*                      get_field(const t_config_option_key& opt_key, int opt_index = -1) const;
    Line*                       get_line(const t_config_option_key& opt_key);
    bool                        set_value(const t_config_option_key& opt_key, const boost::any& value);
    ConfigOptionsGroupShp       new_optgroup(const wxString& title, int noncommon_label_width = -1);
    const ConfigOptionsGroupShp get_optgroup(const wxString& title) const;

    bool set_item_colour(const wxColour *clr) {
        if (m_item_color != clr) {
            m_item_color = clr;
            return true;
        }
        return false;
    }

    const wxColour get_item_colour() {
            return *m_item_color;
    }
    bool get_show() const { return m_show; }

public:
    bool    is_modified_values  { false };
    bool    is_nonsys_values    { true };

    std::vector <ConfigOptionsGroupShp> optgroups;

private:
    wxWindow*   m_parent    { nullptr };
    wxBoxSizer* m_vsizer    { nullptr };
    bool        m_show      { true };
    wxString    m_title;
    size_t      m_iconID;

    Biz::Preset::IConfigInteractor* m_config_interactor {nullptr};

    // Color of TreeCtrlItem. The wxColour will be updated only if the new wxColour pointer differs from the currently rendered one.
    const wxColour*        m_item_color;
};

}
